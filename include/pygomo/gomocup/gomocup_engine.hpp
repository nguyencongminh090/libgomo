#pragma once

#include <cstdint>
#include <mutex>
#include <optional>

#include "pygomo/core/engine_port.hpp"
#include "pygomo/parse/line_parser.hpp"
#include "pygomo/transport/transport.hpp"

namespace pygomo {

/// EnginePort for engines that speak Gomocup / Piskvork with the Yixin extensions.
/// The transport is not owned and must outlive the engine. Call open() once before use.
///
/// The sink is called from the transport's reader thread, never while the engine holds a lock.
/// An `ERROR` line during a search is not the end of it: it is sent raw as a MessageEvent and
/// the search waits for a move (engines print non-fatal errors and still answer). A search ends
/// with a move, swap, an illegal move (ErrorEvent) or a closed connection; an engine that fails
/// and sends no move leaves it open until close(). The engine has no deadline of its own: wrap
/// it in DeadlineEngine (pygomo/engine/deadline_engine.hpp) to get one.
class GomocupEngine final : public EnginePort, private TransportListener {
  public:
    /// Binds the engine to `transport`; nothing is read or sent until open().
    /// `parser` reads every engine line on the reader thread; empty means parseGomocupLine.
    /// To add to the default instead of replacing it, pass chainParsers(custom, parseGomocupLine).
    /// The ERROR rule and the illegal-move check run on the parser's result, so they still hold.
    explicit GomocupEngine(Transport& transport, parse::LineParser parser = {});

    /// Starts reading from the transport. Returns NotReady if the transport cannot start.
    [[nodiscard]] std::expected<void, EngineError> open();

    /// True for valid rules that have a Gomocup rule id: any board rule with free opening, or
    /// freestyle with swap1 or swap2.
    [[nodiscard]] bool supports(const Rules& rules) const override;
    /// Stores the rules for later searches; sends nothing. Unsupported if supports() is false.
    [[nodiscard]] std::expected<void, EngineError> setRules(const Rules& rules) override;
    /// Sends START, the INFO settings and the position (BEGIN, or BOARD..DONE), then returns.
    /// Errors: Unsupported for a non-square board, Busy while a search runs, NotReady after the
    /// connection closed, TransportFailed when a line cannot be written.
    /// A failed write after the search already ended (its terminal event was delivered) is not
    /// an error: the call succeeds, so the caller sees one outcome.
    [[nodiscard]] std::expected<void, EngineError> startSearch(const Position& position,
                                                               const Limits&   limits,
                                                               EventSink&      sink) override;
    /// Sends YXSTOP if a search runs; the engine is expected to answer with a move, which ends it.
    void stop() override;
    /// Writes the line to the engine. Errors: TransportFailed.
    [[nodiscard]] std::expected<void, EngineError> sendRaw(std::string_view line) override;

  private:
    void onLine(std::string_view line) override;
    void onClosed(CloseReason reason) override;

    Transport&              transport_;
    parse::LineParser       parser_;
    std::mutex              mutex_;
    Rules                   rules_{};
    bool                    closed_{false};
    EventSink*              sink_{nullptr};  // non-null while a search runs
    std::optional<Position> position_;
    // counts started searches, so a failed start clears only its own
    std::uint64_t searchId_{0};
};

}  // namespace pygomo
