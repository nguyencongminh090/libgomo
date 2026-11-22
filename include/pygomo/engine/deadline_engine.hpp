#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>

#include "pygomo/core/engine_port.hpp"

namespace pygomo {

/// Gives any EnginePort a search deadline. The wrapped engine is not owned and must outlive
/// this object; destroy this object only after the engine has stopped delivering events
/// (close its transport first).
///
/// A search with `Limits::turnTime` > 0 has a deadline of turnTime + grace. At it the wrapped
/// engine's stop() is called. After one more grace the watchdog commits to delivering
/// ErrorEvent{"engine did not answer"}. The late engine events are dropped. startSearch returns
/// Busy until timeout delivery begins and the inner engine's terminal event has arrived. An
/// engine that never answers stays Busy. No turnTime (or 0) means no deadline.
///
/// The terminal callback starts after all earlier callbacks for that search have returned.
/// The caller can release the sink once the terminal callback returns. Events reach the sink
/// from the wrapped engine's thread, or from the watchdog thread for the timeout error, with
/// the state mutex unlocked. The sink may call startSearch from either thread.
class DeadlineEngine final : public EnginePort {
public:
    /// Tuning for the deadline.
    struct Options {
        /// Slack on top of turnTime, and again after stop().
        std::chrono::milliseconds grace{2000};
    };

    /// Wraps `inner` with the default Options. `inner` must outlive this object.
    explicit DeadlineEngine(EnginePort& inner);
    /// Wraps `inner` with the given Options. `inner` must outlive this object.
    DeadlineEngine(EnginePort& inner, Options options);
    /// Stops and joins the watchdog thread; delivers no further event.
    ~DeadlineEngine() override;
    DeadlineEngine(const DeadlineEngine&) = delete;
    DeadlineEngine& operator=(const DeadlineEngine&) = delete;

    /// Asks the wrapped engine.
    [[nodiscard]] bool supports(const Rules& rules) const override;
    /// Forwards to the wrapped engine; its errors pass through.
    [[nodiscard]] std::expected<void, EngineError> setRules(const Rules& rules) override;
    /// Starts the wrapped engine's search under a deadline. Returns Busy while a search runs or
    /// while a timed-out one waits for callbacks or still owes its answer.
    [[nodiscard]] std::expected<void, EngineError> startSearch(
        const Position& position, const Limits& limits, EventSink& sink) override;
    /// Forwards to the wrapped engine's stop().
    void stop() override;
    /// Forwards to the wrapped engine; its errors pass through.
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
    void finishSearch(std::unique_lock<std::mutex>& lock, const SearchEvent& event);
    void watchdog();

    using Clock = std::chrono::steady_clock;

    EnginePort&             inner_;
    const Options           options_;
    Relay                   relay_{*this};
    std::mutex              mutex_;
    std::condition_variable cv_;
    std::thread             watchdog_;
    bool          quit_{false};
    EventSink*    sink_{nullptr};    // the caller's sink while a search runs
    bool          detached_{false};  // a search timed out and the engine has not answered it yet
    bool terminalPending_{false};
    std::size_t progressCallbacks_{0};
    bool          stopSent_{false};
    std::uint64_t searchId_{0};
    std::optional<Clock::time_point> deadline_;
};

}  // namespace pygomo
