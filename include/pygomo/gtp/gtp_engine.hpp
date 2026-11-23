#pragma once

#include <chrono>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "pygomo/core/command_port.hpp"
#include "pygomo/core/engine_port.hpp"
#include "pygomo/core/search_request.hpp"
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
class GtpEngine final : public EnginePort, public CommandPort, private TransportListener {
  public:
    /// Binds the engine to `transport`; nothing is read or sent until open().
    /// `parser`, if set, runs on the reader thread (outside the lock) during a search for each
    /// non-empty reply body line and non-header free-text line, never on reply headers or blank
    /// terminators. Non-terminal, non-Unparsed results go to the sink and consume the line;
    /// Unparsed or terminal results fall back to the built-in GTP handling. Only a reply ends a
    /// search: a parser that consumes the final `play X` line of an analyze reply loses the move.
    /// `encoder` builds the search commands under the engine lock. It must be pure and must
    /// not call back into the engine. An empty encoder uses encode::defaultGtpRequest with the
    /// current analyze interval. Each line must be a single command without an id that gets
    /// one reply (no blank or comment lines).
    /// The last command requests the move. If it starts with `kata-genmove_analyze`, its reply
    /// may stream `info` lines and ends with `play X`.
    explicit GtpEngine(Transport& transport, parse::LineParser parser = {},
                       RequestEncoder encoder = {});

    /// Starts reading from the transport. Returns NotReady if the transport cannot start.
    [[nodiscard]] std::expected<void, EngineError> open();

    /// Uses `kata-genmove_analyze` in the default script for later searches. The engine streams
    /// `info` lines (sent as InfoEvents, about every `interval`) before the final `play X`.
    /// Off by default. The interval is rounded down to whole centiseconds, at least 1. Still
    /// synchronous: stop() stays a no-op. Safe to call from any thread; a running search keeps
    /// the mode it started with.
    void setAnalyze(std::optional<std::chrono::milliseconds> interval);

    /// Accepts valid rules with free opening and empty extensions by default.
    /// With a custom encoder, accepts any rules that pass validate().
    [[nodiscard]] bool supports(const Rules& rules) const override;
    /// Stores the rules for later searches; sends nothing. Unsupported if supports() is false.
    [[nodiscard]] std::expected<void, EngineError> setRules(const Rules& rules) override;
    /// Sends the encoded commands with consecutive ids and uses the final id for the move.
    /// Returns Busy while a search runs, NotReady after close or TransportFailed on write failure.
    /// An encoding error is returned before any writes. An empty script or a blank or comment
    /// line returns Unsupported. See encode::defaultGtpRequest for default limitations.
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

    /// Writes each line of commandLines(command) as a GTP command, as is (no id). Only the None
    /// completion and commands without `block` or `terminator` are supported: a GTP command is
    /// one line with one reply, and the reply to an arbitrary command is no move for a known
    /// position, so Search, Until, blocks and terminators are Unsupported. Each line's reply is
    /// attributed like a sendRaw reply (AUD-04): dropped, since no search runs when it is sent.
    /// Busy while a search runs. With force during a search: stop() cannot interrupt `genmove`,
    /// so the command is queued and execute returns; it is written from the reader thread right
    /// after the search's natural terminal event has reached the old sink (startSearch and
    /// execute are Busy in between). Wrap in DeadlineEngine to bound the wait. A second force
    /// while one is queued is Busy. A queued command dropped by a close, or whose write fails,
    /// gets one ErrorEvent on its sink; otherwise the sink gets no event.
    /// Errors: Unsupported (nothing sent), NotReady after the connection closed, Busy,
    /// TransportFailed when a line cannot be written.
    [[nodiscard]] std::expected<void, EngineError> execute(const Command& command, EventSink& sink,
                                                           CommandOptions options = {}) override;

  private:
    void onLine(std::string_view line) override;
    void onClosed(CloseReason reason) override;

    /// Handles one complete reply (of the running search or of a raw command). Caller holds the
    /// lock and passes the sink out through `deliver` when an event must be reported.
    void onReply(bool ok, std::optional<int> id, const std::string& text, EventSink*& deliver,
                 std::optional<SearchEvent>& event);

    Transport&              transport_;
    parse::LineParser       parser_;
    bool                    customEncoder_;
    RequestEncoder          encoder_;
    std::mutex              mutex_;
    Rules                   rules_{};
    bool                    closed_{false};
    EventSink*              sink_{nullptr};  // non-null while a search runs
    std::optional<Position> position_;
    int                     nextId_{1};
    int firstId_{0};  // ids of the running search: [firstId_, lastId_]; lastId_ requests the move
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

    // Swallows nothing in practice; its address in sink_ keeps the engine Busy while a queued
    // command is written after a search's terminal event.
    struct DiscardSink final : EventSink {
        void onEvent(const SearchEvent&) override {}
    };
    // A command queued by execute(force), written after the search's terminal event.
    struct Queued {
        std::vector<std::string> lines;
        EventSink*               sink;
    };

    std::expected<void, EngineError> writeCommand(const std::vector<std::string>& lines);
    void                             flushQueued();

    DiscardSink           discard_{};
    std::optional<Queued> queued_{};
};

}  // namespace pygomo
