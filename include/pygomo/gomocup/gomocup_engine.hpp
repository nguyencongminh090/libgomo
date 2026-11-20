#pragma once

#include <mutex>
#include <optional>

#include "pygomo/core/engine_port.hpp"
#include "pygomo/transport/transport.hpp"

namespace pygomo {

/// EnginePort for engines that speak Gomocup / Piskvork with the Yixin extensions.
/// The transport is not owned and must outlive the engine. Call open() once before use.
///
/// The sink is called from the transport's reader thread, never while the engine holds a lock.
/// An `ERROR` line during a search is not the end of it: it is sent raw as a MessageEvent and the search
/// waits for a move (engines print non-fatal errors and still answer). A search ends with a move, swap,
/// an illegal move (ErrorEvent) or a closed connection; an engine that fails and sends no move leaves it
/// open until close(). For a deadline wrap the engine in DeadlineEngine (pygomo/engine/deadline_engine.hpp).
class GomocupEngine final : public EnginePort, private TransportListener {
public:
    explicit GomocupEngine(Transport& transport);

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

    Transport& transport_;
    std::mutex mutex_;
    Rules rules_{};
    bool closed_{false};
    EventSink* sink_{nullptr};  // non-null while a search runs
    std::optional<Position> position_;
};

}  // namespace pygomo
