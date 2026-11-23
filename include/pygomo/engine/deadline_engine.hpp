#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>

#include "pygomo/core/command_port.hpp"
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
/// engine that never answers stays Busy; close it. No turnTime (or 0) means no deadline.
///
/// The terminal callback starts after all earlier callbacks for that search have returned.
/// The caller can release the sink once the terminal callback returns. Events reach the sink
/// from the wrapped engine's thread, or from the watchdog thread for the timeout error, with
/// the state mutex unlocked. The sink may call startSearch from either thread. A progress
/// callback that never returns also holds back the timeout error (and the destructor), and one
/// that makes the wrapped engine deliver its terminal event on the same thread deadlocks.
class DeadlineEngine final : public EnginePort, public CommandPort {
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
    DeadlineEngine(const DeadlineEngine&)            = delete;
    DeadlineEngine& operator=(const DeadlineEngine&) = delete;

    /// Asks the wrapped engine.
    [[nodiscard]] bool supports(const Rules& rules) const override;
    /// Forwards to the wrapped engine; its errors pass through.
    [[nodiscard]] std::expected<void, EngineError> setRules(const Rules& rules) override;
    /// Starts the wrapped engine's search under a deadline. Returns Busy while a search runs or
    /// while a timed-out one waits for callbacks or still owes its answer.
    [[nodiscard]] std::expected<void, EngineError> startSearch(const Position& position,
                                                               const Limits&   limits,
                                                               EventSink&      sink) override;
    /// Forwards to the wrapped engine's stop().
    void stop() override;
    /// Forwards to the wrapped engine; its errors pass through.
    [[nodiscard]] std::expected<void, EngineError> sendRaw(std::string_view line) override;

    /// Forwards to the wrapped engine when it is a CommandPort; Unsupported otherwise. A command
    /// gets no deadline (it has no Limits): bound a Search command with stop() or use Until.
    /// Busy while a timed-out search still owes its answer or its terminal callback runs
    /// (AUD-03), so nothing is written to an engine that may still be thinking.
    /// With force while a search of this object runs: calls the wrapped stop(), keeps the command
    /// and returns. When the search's terminal event has reached its sink, the command is passed
    /// to the wrapped engine from that thread, or from execute's thread if it is still inside
    /// stop() (an error then arrives as ErrorEvent on `sink`);
    /// startSearch and execute are Busy in between. If the search times out instead, the engine
    /// may still be thinking, so the command is never sent: `sink` gets ErrorEvent right after
    /// the timeout error. A second force while one is kept is Busy. Without a search of this
    /// object, force passes through to the wrapped engine. Destroying this object drops a kept
    /// command without an event.
    [[nodiscard]] std::expected<void, EngineError> execute(const Command& command, EventSink& sink,
                                                           CommandOptions options = {}) override;

  private:
    class Relay final : public EventSink {
      public:
        explicit Relay(DeadlineEngine& owner) : owner_(owner) {}
        void onEvent(const SearchEvent& event) override;

      private:
        DeadlineEngine& owner_;
    };

    void onInnerEvent(const SearchEvent& event);
    void finishSearch(std::unique_lock<std::mutex>& lock, const SearchEvent& event,
                      bool timedOut = false);
    void watchdog();

    using Clock = std::chrono::steady_clock;

    EnginePort&             inner_;
    const Options           options_;
    Relay                   relay_{*this};
    std::mutex              mutex_;
    std::condition_variable cv_;
    std::thread             watchdog_;
    bool                    quit_{false};
    EventSink*              sink_{nullptr};  // the caller's sink while a search runs
    bool detached_{false};  // a search timed out and the engine has not answered it yet
    // A terminal event is claimed, waiting for progress callbacks.
    bool terminalPending_{false};
    // Progress callbacks running outside the mutex.
    std::size_t                      progressCallbacks_{0};
    bool                             stopSent_{false};
    std::uint64_t                    searchId_{0};
    std::optional<Clock::time_point> deadline_;

    // A command kept by execute(force) until this object's search ends.
    struct Queued {
        Command       command;
        EventSink*    sink;
        std::uint64_t id;
        bool          stopInFlight;  // execute is still inside inner_.stop()
        bool          delivered;     // the old sink has the search's terminal event
    };

    void passOn(Queued queued);

    CommandPort*          commands_{nullptr};  // inner_ as a CommandPort, or null
    std::optional<Queued> queued_{};
    std::uint64_t         queueId_{0};
    bool                  flushing_{false};  // a kept command waits to be, or is, passed on
};

}  // namespace pygomo
