#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>

#include "pygomo/core/engine_port.hpp"

namespace pygomo {

/// Gives any EnginePort a search deadline. The wrapped engine is not owned and must outlive this object; destroy
/// this object only after the engine has stopped delivering events (close its transport first).
///
/// A search with `Limits::turnTime` > 0 has a deadline of turnTime + grace. At it the wrapped engine's stop() is
/// called; after one more grace the sink gets ErrorEvent{"engine did not answer"} (from a watchdog thread, never
/// under a lock). The engine still owes its answer: until its late terminal event arrives, which is dropped,
/// startSearch returns Busy without calling the engine, so a late answer can never end a later search. An engine
/// that never answers stays Busy; close it. No turnTime (or 0) means no deadline.
class DeadlineEngine final : public EnginePort {
public:
    struct Options {
        std::chrono::milliseconds grace{2000};  ///< Slack on top of turnTime, and again after stop().
    };

    explicit DeadlineEngine(EnginePort& inner);
    DeadlineEngine(EnginePort& inner, Options options);
    ~DeadlineEngine() override;
    DeadlineEngine(const DeadlineEngine&) = delete;
    DeadlineEngine& operator=(const DeadlineEngine&) = delete;

    [[nodiscard]] bool supports(const Rules& rules) const override;
    [[nodiscard]] std::expected<void, EngineError> setRules(const Rules& rules) override;
    [[nodiscard]] std::expected<void, EngineError> startSearch(
        const Position& position, const Limits& limits, EventSink& sink) override;
    void stop() override;
    [[nodiscard]] std::expected<void, EngineError> sendRaw(std::string_view line) override;

private:
    class Relay final : public EventSink {
    public:
        explicit Relay(DeadlineEngine& owner) : owner_(owner) {}
        void onEvent(const SearchEvent& event) override;

    private:
        DeadlineEngine& owner_;
    };

    void onInnerEvent(const SearchEvent& event);
    void watchdog();

    using Clock = std::chrono::steady_clock;

    EnginePort& inner_;
    const Options options_;
    Relay relay_{*this};
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread watchdog_;
    bool quit_{false};
    EventSink* sink_{nullptr};  // the caller's sink while a search runs
    bool detached_{false};      // a search timed out and the engine has not answered it yet
    bool stopSent_{false};
    std::uint64_t searchId_{0};
    std::optional<Clock::time_point> deadline_;
};

}  // namespace pygomo
