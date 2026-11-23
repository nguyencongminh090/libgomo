#include <doctest.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>
#include <vector>

#include "gomo/transport/stream_transport.hpp"

using namespace gomo;
using namespace std::chrono_literals;

namespace {

/// An input stream that blocks like a pipe: the reader waits until data is fed or the
/// "peer" finishes.
class PipeBuf : public std::streambuf {
  public:
    void feed(std::string data) {
        {
            std::scoped_lock lock(mutex_);
            pending_ += data;
        }
        cv_.notify_all();
    }

    void finish() {
        {
            std::scoped_lock lock(mutex_);
            done_ = true;
        }
        cv_.notify_all();
    }

  protected:
    int_type underflow() override {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return !pending_.empty() || done_; });
        if (pending_.empty()) return traits_type::eof();
        current_ = std::move(pending_);
        pending_.clear();
        setg(current_.data(), current_.data(), current_.data() + current_.size());
        return traits_type::to_int_type(current_[0]);
    }

  private:
    std::mutex              mutex_;
    std::condition_variable cv_;
    std::string             pending_;
    std::string             current_;
    bool                    done_{false};
};

class Recorder final : public TransportListener {
  public:
    void onLine(std::string_view line) override {
        std::scoped_lock lock(mutex_);
        lines_.emplace_back(line);
        if (!closed_.empty()) ++eventsAfterClose_;
    }

    void onClosed(CloseReason reason) override {
        {
            std::scoped_lock lock(mutex_);
            closed_.push_back(reason);
        }
        cv_.notify_all();
    }

    bool waitClosed() {
        std::unique_lock lock(mutex_);
        return cv_.wait_for(lock, 5s, [this] { return !closed_.empty(); });
    }

    std::vector<std::string> lines() {
        std::scoped_lock lock(mutex_);
        return lines_;
    }
    std::vector<CloseReason> closed() {
        std::scoped_lock lock(mutex_);
        return closed_;
    }
    int eventsAfterClose() {
        std::scoped_lock lock(mutex_);
        return eventsAfterClose_;
    }

  private:
    std::mutex               mutex_;
    std::condition_variable  cv_;
    std::vector<std::string> lines_;
    std::vector<CloseReason> closed_;
    int                      eventsAfterClose_{0};
};

}  // namespace

TEST_CASE("transport_lines: CRLF stripped, empty lines kept, final line without newline kept") {
    std::istringstream in(std::string("a\r\nb\n\nlast"));
    std::ostringstream out;
    Recorder           rec;
    StreamTransport    transport(in, out);

    REQUIRE(transport.start(rec).has_value());
    REQUIRE(rec.waitClosed());

    CHECK(rec.lines() == std::vector<std::string>{"a", "b", "", "last"});
    CHECK(rec.closed() == std::vector<CloseReason>{CloseReason::Eof});
}

TEST_CASE("transport_lines: input ending with a newline adds no extra empty line") {
    std::istringstream in("one\ntwo\n");
    std::ostringstream out;
    Recorder           rec;
    StreamTransport    transport(in, out);

    REQUIRE(transport.start(rec).has_value());
    REQUIRE(rec.waitClosed());
    CHECK(rec.lines() == std::vector<std::string>{"one", "two"});
}

TEST_CASE("transport_write: concurrent writers never interleave lines") {
    constexpr int kThreads   = 8;
    constexpr int kPerThread = 100;

    PipeBuf            pipe;
    std::istream       in(&pipe);
    std::ostringstream out;
    Recorder           rec;
    {
        StreamTransport transport(in, out);
        REQUIRE(transport.start(rec).has_value());

        std::vector<std::thread> writers;
        for (int t = 0; t < kThreads; ++t) {
            writers.emplace_back([&transport, t] {
                for (int n = 0; n < kPerThread; ++n) {
                    const std::string line = "t" + std::to_string(t) + "-n" + std::to_string(n);
                    CHECK(transport.writeLine(line).has_value());
                }
            });
        }
        for (auto& w : writers) w.join();

        pipe.finish();
        REQUIRE(rec.waitClosed());
    }

    std::set<std::string> expected;
    for (int t = 0; t < kThreads; ++t) {
        for (int n = 0; n < kPerThread; ++n) {
            expected.insert("t" + std::to_string(t) + "-n" + std::to_string(n));
        }
    }

    std::istringstream    written(out.str());
    std::set<std::string> seen;
    std::string           line;
    int                   count = 0;
    while (std::getline(written, line)) {
        ++count;
        seen.insert(line);
    }
    CHECK(count == kThreads * kPerThread);
    CHECK(seen == expected);
}

TEST_CASE("transport_close: close ends writing, notifies once and silences the listener") {
    PipeBuf            pipe;
    std::istream       in(&pipe);
    std::ostringstream out;
    Recorder           rec;
    {
        StreamTransport transport(in, out);
        REQUIRE(transport.start(rec).has_value());

        pipe.feed("before\n");
        // Wait for the first line so the order of events is deterministic.
        for (int i = 0; i < 500 && rec.lines().empty(); ++i) std::this_thread::sleep_for(10ms);
        REQUIRE(rec.lines() == std::vector<std::string>{"before"});

        transport.close();
        transport.close();  // idempotent

        auto w = transport.writeLine("after");
        REQUIRE_FALSE(w.has_value());
        CHECK(w.error() == TransportError::Closed);

        pipe.feed("late\n");  // must never reach the listener
        pipe.finish();
    }  // destructor joins the reader once the pipe has ended

    CHECK(rec.closed() == std::vector<CloseReason>{CloseReason::Requested});
    CHECK(rec.lines() == std::vector<std::string>{"before"});
    CHECK(rec.eventsAfterClose() == 0);
    CHECK(out.str().empty());
}

TEST_CASE("transport_limit: a line over the limit ends the transport") {
    std::istringstream in("ok\n123456789\nafter\n");
    std::ostringstream out;
    Recorder           rec;
    StreamTransport    transport(in, out, StreamTransportOptions{.maxLineLength = 8});

    REQUIRE(transport.start(rec).has_value());
    REQUIRE(rec.waitClosed());

    CHECK(rec.lines() == std::vector<std::string>{"ok"});
    CHECK(rec.closed() == std::vector<CloseReason>{CloseReason::LineTooLong});
}

TEST_CASE("transport_limit: a line exactly at the limit is accepted") {
    std::istringstream in("12345678\n");
    std::ostringstream out;
    Recorder           rec;
    StreamTransport    transport(in, out, StreamTransportOptions{.maxLineLength = 8});

    REQUIRE(transport.start(rec).has_value());
    REQUIRE(rec.waitClosed());

    CHECK(rec.lines() == std::vector<std::string>{"12345678"});
    CHECK(rec.closed() == std::vector<CloseReason>{CloseReason::Eof});
}

TEST_CASE("transport_misc: write before start and double start are rejected") {
    // The pipe stays open, so the transport is still running when start() is called again.
    PipeBuf            pipe;
    std::istream       in(&pipe);
    std::ostringstream out;
    Recorder           rec;
    {
        StreamTransport transport(in, out);

        auto early = transport.writeLine("x");
        REQUIRE_FALSE(early.has_value());
        CHECK(early.error() == TransportError::NotStarted);

        REQUIRE(transport.start(rec).has_value());
        auto again = transport.start(rec);
        REQUIRE_FALSE(again.has_value());
        CHECK(again.error() == TransportError::AlreadyStarted);

        pipe.finish();
        REQUIRE(rec.waitClosed());
    }
}

TEST_CASE("transport_misc: start after the transport ended reports Closed") {
    std::istringstream in("");
    std::ostringstream out;
    Recorder           rec;
    StreamTransport    transport(in, out);

    transport.close();
    auto r = transport.start(rec);
    REQUIRE_FALSE(r.has_value());
    CHECK(r.error() == TransportError::Closed);
}

TEST_CASE("transport_misc: a line with an embedded newline is refused, nothing is written") {
    PipeBuf            pipe;
    std::istream       in(&pipe);
    std::ostringstream out;
    Recorder           rec;
    {
        StreamTransport transport(in, out);
        REQUIRE(transport.start(rec).has_value());

        auto r = transport.writeLine("one\ntwo");
        REQUIRE_FALSE(r.has_value());
        CHECK(r.error() == TransportError::InvalidLine);

        pipe.finish();
        REQUIRE(rec.waitClosed());
    }
    CHECK(out.str().empty());
}

TEST_CASE("transport_misc: a failed output stream reports IoError") {
    PipeBuf            pipe;
    std::istream       in(&pipe);
    std::ostringstream out;
    out.setstate(std::ios::failbit);
    Recorder rec;
    {
        StreamTransport transport(in, out);
        REQUIRE(transport.start(rec).has_value());

        auto r = transport.writeLine("x");
        REQUIRE_FALSE(r.has_value());
        CHECK(r.error() == TransportError::IoError);

        pipe.finish();
        REQUIRE(rec.waitClosed());
    }
}

TEST_CASE("transport_misc: a listener may call writeLine from onLine") {
    struct Echo final : TransportListener {
        Transport*              transport{};
        std::mutex              mutex;
        std::condition_variable cv;
        bool                    closed{false};
        void                    onLine(std::string_view line) override {
            CHECK(transport->writeLine(std::string("echo ") + std::string(line)).has_value());
        }
        void onClosed(CloseReason) override {
            {
                std::scoped_lock lock(mutex);
                closed = true;
            }
            cv.notify_all();
        }
    };

    std::istringstream in("hi\n");
    std::ostringstream out;
    Echo               echo;
    StreamTransport    transport(in, out);
    echo.transport = &transport;

    REQUIRE(transport.start(echo).has_value());
    {
        std::unique_lock lock(echo.mutex);
        REQUIRE(echo.cv.wait_for(lock, 5s, [&] { return echo.closed; }));
    }
    CHECK(out.str() == "echo hi\n");
}
