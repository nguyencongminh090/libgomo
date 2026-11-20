#pragma once

#include <mutex>
#include <optional>

#include "pygomo/core/engine_port.hpp"
#include "pygomo/transport/transport.hpp"

namespace pygomo {

/// EnginePort for KataGomo's `gom` mode (gomprotocol.cpp): GTP-style rule commands plus Gomocup
/// START / BOARD..DONE. Derived from reading the source; no real engine has been run.
/// The transport is not owned and must outlive the engine. Call open() once before use.
///
/// The engine reads one line at a time, so stop() cannot interrupt a search and is a no-op.
/// The sink is called from the transport's reader thread, never while the engine holds a lock.
class KataGomoEngine final : public EnginePort, private TransportListener {
public:
    explicit KataGomoEngine(Transport& transport);

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
