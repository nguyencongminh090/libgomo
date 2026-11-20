#include <doctest.h>

#include <signal.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <optional>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "pygomo/process/process_transport.hpp"

using namespace pygomo;
using namespace std::chrono_literals;

namespace {

struct Recorder final : TransportListener {
    void onLine(std::string_view l) override {
        std::lock_guard lock(m);
        lines.emplace_back(l);
        cv.notify_all();
    }
    void onClosed(CloseReason r) override {
        std::lock_guard lock(m);
        reasons.push_back(r);
        cv.notify_all();
    }
    bool waitClosed(std::chrono::milliseconds t = 5s) {
        std::unique_lock lock(m);
        return cv.wait_for(lock, t, [&] { return !reasons.empty(); });
    }
    bool waitLines(std::size_t n, std::chrono::milliseconds t = 5s) {
        std::unique_lock lock(m);
        return cv.wait_for(lock, t, [&] { return lines.size() >= n; });
    }
    std::mutex m;
    std::condition_variable cv;
    std::vector<std::string> lines;
    std::vector<CloseReason> reasons;
};

ProcessOptions sh(const std::string& script) {
    ProcessOptions o;
    o.program = "/bin/sh";
    o.args    = {"-c", script};
    return o;
}

}  // namespace

TEST_CASE("proc_lines: CRLF, empty line and final line without newline") {
    ProcessTransport t(sh("printf 'a\\r\\nb\\n\\nlast'"));
    Recorder r;
    REQUIRE(t.start(r));
    REQUIRE(r.waitClosed());
    CHECK(r.lines == std::vector<std::string>{"a", "b", "", "last"});
    CHECK(r.reasons == std::vector<CloseReason>{CloseReason::Eof});
}

TEST_CASE("proc_roundtrip: 8 threads x 100 lines through cat") {
    ProcessOptions o;
    o.program = "cat";
    ProcessTransport t(o);
    Recorder r;
    REQUIRE(t.start(r));
    std::atomic<int> writeFailures{0};
    std::vector<std::thread> ws;
    for (int w = 0; w < 8; ++w) {
        ws.emplace_back([&, w] {
            for (int i = 0; i < 100; ++i) {
                if (!t.writeLine("line-" + std::to_string(w) + "-" + std::to_string(i) + "-end")) {
                    ++writeFailures;
                }
            }
        });
    }
    for (auto& w : ws) w.join();
    CHECK(writeFailures == 0);
    REQUIRE(r.waitLines(800));
    t.close();
    std::set<std::string> seen;
    {
        std::lock_guard lock(r.m);
        seen.insert(r.lines.begin(), r.lines.end());
    }
    CHECK(seen.size() == 800);
    for (int w = 0; w < 8; ++w) {
        for (int i = 0; i < 100; ++i) {
            CHECK(seen.count("line-" + std::to_string(w) + "-" + std::to_string(i) + "-end") == 1);
        }
    }
}

TEST_CASE("proc_close: a blocked child is ended and reaped, later writes fail") {
    ProcessTransport t(sh("exec sleep 60"));
    Recorder r;
    REQUIRE(t.start(r));
    auto begin = std::chrono::steady_clock::now();
    t.close();
    CHECK(std::chrono::steady_clock::now() - begin < 3s);
    REQUIRE(r.waitClosed(100ms));
    CHECK(r.reasons == std::vector<CloseReason>{CloseReason::Requested});
    REQUIRE(t.exitStatus().has_value());
    auto w = t.writeLine("x");
    REQUIRE_FALSE(w);
    CHECK(w.error() == TransportError::Closed);
    t.close();  // idempotent
    CHECK(r.reasons.size() == 1);
}

TEST_CASE("proc_exit: exit code is reported") {
    ProcessTransport t(sh("exit 3"));
    Recorder r;
    REQUIRE(t.start(r));
    REQUIRE(r.waitClosed());
    CHECK(r.reasons == std::vector<CloseReason>{CloseReason::Eof});
    CHECK(t.exitStatus() == 3);
}

TEST_CASE("proc_spawnfail: a missing executable is an error with no listener call") {
    ProcessOptions o;
    o.program = "/nonexistent/pygomo-engine";
    ProcessTransport t(o);
    Recorder r;
    auto s = t.start(r);
    REQUIRE_FALSE(s);
    std::this_thread::sleep_for(50ms);
    CHECK(r.lines.empty());
    CHECK(r.reasons.empty());
}

TEST_CASE("proc_limit: an over-long line ends the transport and the child") {
    ProcessOptions o = sh("printf 'ok\\n'; head -c 100 /dev/zero | tr '\\0' x; printf '\\n'; "
                          "exec sleep 60");
    o.maxLineLength = 16;
    ProcessTransport t(o);
    Recorder r;
    REQUIRE(t.start(r));
    REQUIRE(r.waitClosed());
    CHECK(r.reasons == std::vector<CloseReason>{CloseReason::LineTooLong});
    CHECK(r.lines == std::vector<std::string>{"ok"});
    CHECK(t.exitStatus().has_value());
}

namespace {

// A zombie still answers kill(pid, 0), so read the state from /proc.
bool running(long pid) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/stat");
    std::string line;
    if (!std::getline(f, line)) return false;
    auto rp = line.rfind(')');
    return rp != std::string::npos && rp + 2 < line.size() && line[rp + 2] != 'Z';
}

}  // namespace

TEST_CASE("proc_group: close() also ends grandchildren of the child") {
    ProcessTransport t(sh("sleep 60 & echo $!; wait"));
    Recorder r;
    REQUIRE(t.start(r));
    REQUIRE(r.waitLines(1));
    long grandchild = std::stol(r.lines.front());
    REQUIRE(running(grandchild));
    t.close();
    bool gone = false;
    for (int i = 0; i < 60 && !gone; ++i) {
        gone = !running(grandchild);
        if (!gone) std::this_thread::sleep_for(50ms);
    }
    CHECK(gone);
    if (!gone) ::kill(static_cast<pid_t>(grandchild), SIGKILL);
}

TEST_CASE("proc_orphan: close() ends grandchildren of a child that exited by itself") {
    ProcessTransport t(sh("sleep 60 & echo $!; exit 0"));
    Recorder r;
    REQUIRE(t.start(r));
    REQUIRE(r.waitLines(1));
    long grandchild = std::stol(r.lines.front());
    REQUIRE(running(grandchild));
    t.close();
    bool gone = false;
    for (int i = 0; i < 60 && !gone; ++i) {
        gone = !running(grandchild);
        if (!gone) std::this_thread::sleep_for(50ms);
    }
    CHECK(gone);
    if (!gone) ::kill(static_cast<pid_t>(grandchild), SIGKILL);
}

TEST_CASE("proc_sigpipe: writing to a child that closed stdin gives IoError, not a signal") {
    ProcessTransport t(sh("exec 0<&-; sleep 5"));
    Recorder r;
    REQUIRE(t.start(r));
    std::optional<TransportError> err;
    for (int i = 0; i < 200 && !err; ++i) {
        auto w = t.writeLine("x");
        if (!w) err = w.error();
        else std::this_thread::sleep_for(10ms);
    }
    REQUIRE(err.has_value());
    CHECK(*err == TransportError::IoError);
    t.close();  // before the Recorder goes away
}

TEST_CASE("proc_nohandler: the process-wide SIGPIPE disposition is left alone") {
    struct sigaction before {}, after {};
    REQUIRE(::sigaction(SIGPIPE, nullptr, &before) == 0);
    {
        ProcessTransport t(sh("exit 0"));
        Recorder r;
        REQUIRE(t.start(r));
        REQUIRE(r.waitClosed());
        t.close();
    }
    REQUIRE(::sigaction(SIGPIPE, nullptr, &after) == 0);
    CHECK(after.sa_handler == before.sa_handler);
    CHECK((after.sa_flags & SA_SIGINFO) == (before.sa_flags & SA_SIGINFO));
}
