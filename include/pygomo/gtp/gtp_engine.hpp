#pragma once

#include <chrono>
#include <deque>
#include <mutex>
#include <optional>
#include <string>

#include "pygomo/core/engine_port.hpp"
#include "pygomo/parse/line_parser.hpp"
#include "pygomo/transport/transport.hpp"

namespace pygomo {

/// EnginePort for engines that speak GTP with the KataGomo Gomoku extensions
/// (`FREESTYLE`/`STANDARD`/`RENJU`, `maxmoves`, `kata-set-param`).
/// The transport is not owned and must outlive the engine. Call open() once before use.
///
/// The sink is called from the transport's reader thread, never while the engine holds a lock.
/// While a search runs, a line outside any reply is sent raw as a MessageEvent; outside a
/// search it is dropped. The engine has no deadline of its own; see DeadlineEngine.
///
/// stop() cannot interrupt a search: a GTP engine reads commands in one thread, so a line sent
/// during `genmove` is only read after the move is chosen. Searches end by the limits given to
/// startSearch.
class GtpEngine final : public EnginePort, private TransportListener {
  public:
    /// Binds the engine to `transport`; nothing is read or sent until open().
    /// `parser`, if set, runs on the reader thread (outside the lock) during a search for each
    /// non-empty reply body line and non-header free-text line, never on reply headers or blank
    /// terminators. Non-terminal, non-Unparsed results go to the sink and consume the line;
    /// Unparsed or terminal results fall back to the built-in GTP handling. Only a reply ends a
    /// search: a parser that consumes the final `play X` line of an analyze reply loses the move.
    explicit GtpEngine(Transport& transport, parse::LineParser parser = {});

    /// Starts reading from the transport. Returns NotReady if the transport cannot start.
    [[nodiscard]] std::expected<void, EngineError> open();

    /// Uses `kata-genmove_analyze` instead of `genmove` for later searches: the engine streams
    /// `info` lines (sent as InfoEvents, about every `interval`) before the final `play X`.
    /// Off by default. The interval is rounded down to whole centiseconds, at least 1. Still
    /// synchronous: stop() stays a no-op. Safe to call from any thread; a running search keeps
    /// the mode it started with.
    void setAnalyze(std::optional<std::chrono::milliseconds> interval);

    /// True for valid rules with free opening; GTP has no swap opening.
    [[nodiscard]] bool supports(const Rules& rules) const override;
    /// Stores the rules for later searches; sends nothing. Unsupported if supports() is false.
    [[nodiscard]] std::expected<void, EngineError> setRules(const Rules& rules) override;
    /// Sends boardsize, clear_board, the rule and limit commands, every move as `play`, then
    /// `genmove` (or `kata-genmove_analyze`), each with an id; then returns. Errors: Unsupported
    /// for matchTime, maxDepth or a position that has no GTP vertex, Busy while a search runs,
    /// NotReady after the connection closed, TransportFailed when a line cannot be written.
    /// A failed write after the search already ended (its terminal event was delivered) is not
    /// an error: the call succeeds, so the caller sees one outcome.
    [[nodiscard]] std::expected<void, EngineError> startSearch(const Position& position,
                                                               const Limits&   limits,
                                                               EventSink&      sink) override;
    /// Does nothing: a running `genmove` cannot be interrupted (see the class comment).
    void stop() override;
    /// Writes the line to the engine as is. GTP answers in order, so its reply is known by its
    /// place: a MessageEvent during a search, dropped otherwise (docs/audit/AUD-04). Errors:
    /// TransportFailed.
    [[nodiscard]] std::expected<void, EngineError> sendRaw(std::string_view line) override;

  private:
    void onLine(std::string_view line) override;
    void onClosed(CloseReason reason) override;

    /// Handles one complete reply (of the running search or of a raw command). Caller holds the
    /// lock and passes the sink out through `deliver` when an event must be reported.
    void onReply(bool ok, std::optional<int> id, const std::string& text, EventSink*& deliver,
                 std::optional<SearchEvent>& event);

    Transport&              transport_;
    parse::LineParser       parser_;
    std::mutex              mutex_;
    Rules                   rules_{};
    bool                    closed_{false};
    EventSink*              sink_{nullptr};  // non-null while a search runs
    std::optional<Position> position_;
    int                     nextId_{1};
    int firstId_{0};  // ids of the running search: [firstId_, lastId_]; lastId_ is the genmove
    int lastId_{0};
    std::optional<std::chrono::milliseconds> analyzeInterval_{};  // setting for the next search
    bool                                     analyzing_{false};   // the running search uses analyze
    // Unanswered commands in write order, true for a raw one (AUD-04). writeMutex_ is taken
    // before mutex_ and held over a queue push and its write, so the queue matches the wire.
    std::deque<bool> pending_{};
    std::mutex       writeMutex_;

    // Reply framing: a header line, then body lines, ended by a blank line.
    bool               inReply_{false};
    bool               replyOk_{false};
    std::optional<int> replyId_{};
    std::string        replyText_{};
};

}  // namespace pygomo
