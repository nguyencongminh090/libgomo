// Test support for EXT-10 command tests: a fake transport and a sink that log into one ordered
// record, so a test can check wire writes against delivered events.
#pragma once

#include <chrono>
#include <cstddef>
#include <expected>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <variant>
#include <vector>

#include "pygomo/core/engine_port.hpp"
#include "pygomo/transport/transport.hpp"

namespace pygomo::testing {

using Lines = std::vector<std::string>;

/// One ordered record of wire writes ("w <line>") and sink events ("<sink> <kind>").
struct Log {
    void add(std::string entry) {
        std::lock_guard lock(m);
        entries.push_back(std::move(entry));
    }
    Lines get() {
        std::lock_guard lock(m);
        return entries;
    }
    std::mutex m;
    Lines      entries;
};

/// Records what the adapter writes, also into the shared log; the test plays the engine.
class FakeTransport final : public Transport {
  public:
    explicit FakeTransport(Log& log) : log_(log) {}
    std::expected<void, TransportError> start(TransportListener& l) override {
        listener = &l;
        return {};
    }
    std::expected<void, TransportError> writeLine(std::string_view line) override {
        std::function<void(const std::string&)> hook;
        {
            std::lock_guard lock(m);
            if (failOn && line == *failOn) return std::unexpected(TransportError::Closed);
            written.emplace_back(line);
            hook = onWrite;
        }
        log_.add("w " + std::string{line});
        // Outside the lock: a hook may answer on this thread, as a fast engine can.
        if (hook) hook(std::string{line});
        return {};
    }
    void  close() override {}
    void  say(std::string_view l) { listener->onLine(l); }
    void  closeNow() { listener->onClosed(CloseReason::Eof); }
    Lines lines() {
        std::lock_guard lock(m);
        return written;
    }

    std::mutex                              m;
    Lines                                   written;
    std::function<void(const std::string&)> onWrite;
    std::optional<std::string>              failOn;  // this line fails to write
    TransportListener*                      listener = nullptr;

  private:
    Log& log_;
};

inline std::string kind(const SearchEvent& e) {
    if (std::holds_alternative<BestMoveEvent>(e)) return "best";
    if (std::holds_alternative<ErrorEvent>(e)) return "error";
    if (std::holds_alternative<MessageEvent>(e)) return "message";
    if (std::holds_alternative<InfoEvent>(e)) return "info";
    return "other";
}

/// Logs each event as "<name> <kind>"; `hook` runs after recording, on the delivering thread.
struct LogSink final : EventSink {
    LogSink(Log& l, std::string n) : log(l), name(std::move(n)) {}
    void onEvent(const SearchEvent& e) override {
        {
            std::lock_guard lock(m);
            events.push_back(e);
        }
        log.add(name + " " + kind(e));
        if (hook) hook(e);
    }
    std::vector<SearchEvent> get() {
        std::lock_guard lock(m);
        return events;
    }
    Log&                                    log;
    std::string                             name;
    std::mutex                              m;
    std::vector<SearchEvent>                events;
    std::function<void(const SearchEvent&)> hook;
};

/// Waits up to two seconds for `s` to hold `n` events.
inline bool waitFor(LogSink& s, std::size_t n) {
    for (int i = 0; i < 400; ++i) {
        if (s.get().size() >= n) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds{5});
    }
    return false;
}

/// Entries of `log` from the first one equal to `from` on; empty if none is.
inline Lines after(const Lines& log, const std::string& from) {
    for (std::size_t i = 0; i < log.size(); ++i) {
        if (log[i] == from) return Lines(log.begin() + static_cast<std::ptrdiff_t>(i), log.end());
    }
    return {};
}

}  // namespace pygomo::testing
