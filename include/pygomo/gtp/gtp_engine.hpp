#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <string>

#include "pygomo/core/engine_port.hpp"
#include "pygomo/transport/transport.hpp"

namespace pygomo {

/// EnginePort for engines that speak GTP with the KataGomo Gomoku extensions
/// (`FREESTYLE`/`STANDARD`/`RENJU`, `maxmoves`, `kata-set-param`).
/// The transport is not owned and must outlive the engine. Call open() once before use.
///
/// The sink is called from the transport's reader thread, never while the engine holds a lock.
///
/// stop() cannot interrupt a search: a GTP engine reads commands in one thread, so a line sent during
/// `genmove` is only read after the move is chosen. Searches end by the limits given to startSearch.
class GtpEngine final : public EnginePort, private TransportListener {
public:
    explicit GtpEngine(Transport& transport);

    /// Starts reading from the transport.
    [[nodiscard]] std::expected<void, EngineError> open();

    /// Uses `kata-genmove_analyze` instead of `genmove` for later searches: the engine streams `info`
    /// lines (sent as InfoEvents, about every `interval`) before the final `play X`. Off by default.
    /// The interval is rounded to whole centiseconds, at least 1. Still synchronous: stop() stays a no-op.
    void setAnalyze(std::optional<std::chrono::milliseconds> interval);

    [[nodiscard]] bool supports(const Rules& rules) const override;
    [[nodiscard]] std::expected<void, EngineError> setRules(const Rules& rules) override;
    [[nodiscard]] std::expected<void, EngineError> startSearch(
        const Position& position, const Limits& limits, EventSink& sink) override;
    void stop() override;
    [[nodiscard]] std::expected<void, EngineError> sendRaw(std::string_view line) override;

private:
    void onLine(std::string_view line) override;
    void onClosed(CloseReason reason) override;

    /// Handles one complete reply of the running search. Caller holds the lock and passes the sink
    /// out through `deliver` when an event must be reported.
    void onReply(bool ok, std::optional<int> id, const std::string& text, EventSink*& deliver,
                 std::optional<SearchEvent>& event);

    Transport& transport_;
    std::mutex mutex_;
    Rules rules_{};
    bool closed_{false};
    EventSink* sink_{nullptr};  // non-null while a search runs
    std::optional<Position> position_;
    int nextId_{1};
    int firstId_{0};  // ids of the running search: [firstId_, lastId_]; lastId_ is the genmove
    int lastId_{0};
    std::optional<std::chrono::milliseconds> analyzeInterval_{};  // setting for the next search
    bool analyzing_{false};                                       // the running search uses analyze

    // Reply framing: a header line, then body lines, ended by a blank line.
    bool inReply_{false};
    bool replyOk_{false};
    std::optional<int> replyId_{};
    std::string replyText_{};
};

}  // namespace pygomo
