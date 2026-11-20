#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>

#include "pygomo/core/engine_port.hpp"
#include "pygomo/transport/transport.hpp"

namespace pygomo {

/// EnginePort for engines that speak Gomocup / Piskvork with the Yixin extensions.
/// The transport is not owned and must outlive the engine. Call open() once before use.
///
/// The sink is called from the transport's reader thread, never while the engine holds a lock.
/// An `ERROR` line during a search is not the end of it: it is sent raw as a MessageEvent and the search
/// waits for a move (engines print non-fatal errors and still answer). A search ends with a move, swap,
/// an illegal move (ErrorEvent) or a closed connection.
///
/// A search with `Limits::turnTime` > 0 also has a deadline. After turnTime + grace without a move the engine
/// writes YXSTOP; after one more grace it ends the search with ErrorEvent{"engine did not answer"} (from a
/// watchdog thread, never under a lock) and accepts a new search. A move that arrives later is dropped, or, if
/// a new search already runs, taken as its answer: close an engine that did not answer. No turnTime (or 0)
/// means no deadline.
class GomocupEngine final : public EnginePort, private TransportListener {
public:
    struct Options {
        std::chrono::milliseconds grace{2000};  ///< Slack on top of turnTime, and again after YXSTOP.
    };

    explicit GomocupEngine(Transport& transport);
    GomocupEngine(Transport& transport, Options options);
    ~GomocupEngine() override;
    GomocupEngine(const GomocupEngine&) = delete;
    GomocupEngine& operator=(const GomocupEngine&) = delete;

    /// Starts reading from the transport.
    [[nodiscard]] std::expected<void, EngineError> open();

    [[nodiscard]] bool supports(const Rules& rules) const override;
    [[nodiscard]] std::expected<void, EngineError> setRules(const Rules& rules) override;
    [[nodiscard]] std::expected<void, EngineError> startSearch(
        const Position& position, const Limits& limits, EventSink& sink) override;
    void stop() override;
    [[nodiscard]] std::expected<void, EngineError> sendRaw(std::string_view line) override;

private:
    void onLine(std::string_view line) override;
    void onClosed(CloseReason reason) override;

    void watchdog();
    void endSearchLocked();  // sink_ = nullptr and no deadline; mutex_ held

    using Clock = std::chrono::steady_clock;

    Transport& transport_;
    const Options options_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread watchdog_;
    bool quit_{false};
    std::uint64_t searchId_{0};
    std::optional<Clock::time_point> deadline_;  // set while a search with a turn time runs
    bool stopSent_{false};
    Rules rules_{};
    bool closed_{false};
    EventSink* sink_{nullptr};  // non-null while a search runs
    std::optional<Position> position_;
};

}  // namespace pygomo
