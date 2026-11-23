#pragma once

#include <cstdint>
#include <functional>
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

/// EnginePort for engines that speak Gomocup / Piskvork with the Yixin extensions.
/// The transport is not owned and must outlive the engine. Call open() once before use.
///
/// The sink is called from the transport's reader thread, never while the engine holds a lock.
/// An `ERROR` line during a search is not the end of it: it is sent raw as a MessageEvent and
/// the search waits for a move (engines print non-fatal errors and still answer). A search ends
/// with a move, swap, an illegal move (ErrorEvent) or a closed connection; an engine that fails
/// and sends no move leaves it open until close(). The engine has no deadline of its own: wrap
/// it in DeadlineEngine (pygomo/engine/deadline_engine.hpp) to get one.
class GomocupEngine final : public EnginePort, public CommandPort, private TransportListener {
  public:
    /// Binds the engine to `transport`; nothing is read or sent until open().
    /// `parser` reads every engine line on the reader thread; empty means parseGomocupLine.
    /// To add to the default instead of replacing it, pass chainParsers(custom, parseGomocupLine).
    /// The ERROR rule and the illegal-move check run on the parser's result, so they still hold.
    /// `encoder` builds the lines of each search; empty means encode::defaultGomocupRequest.
    /// It runs under the engine's lock, so it must be pure and must not call back into it.
    explicit GomocupEngine(Transport& transport, parse::LineParser parser = {},
                           RequestEncoder encoder = {});

    /// Starts reading from the transport. Returns NotReady if the transport cannot start.
    [[nodiscard]] std::expected<void, EngineError> open();

    /// Without a custom encoder: accepts valid rules with empty extensions and a Gomocup rule
    /// id (any board rule with free opening, or freestyle with swap1 or swap2).
    /// With a custom encoder: accepts any valid rules; the encoder refuses what it cannot send
    /// when startSearch runs (Unsupported).
    [[nodiscard]] bool supports(const Rules& rules) const override;
    /// Stores the rules for later searches; sends nothing. Unsupported if supports() is false.
    [[nodiscard]] std::expected<void, EngineError> setRules(const Rules& rules) override;
    /// Sends the encoder's lines (by default START, or RECTSTART w,h for a non-square board, the
    /// INFO settings and the position: BEGIN, or BOARD..DONE), then returns.
    /// An engine without RECTSTART prints ERROR, which does not end a search (GOMO-04): the
    /// search stays open until a DeadlineEngine fires. Rapfi's FLIPY_X mode swaps w and h.
    /// Errors: Busy while a search runs, NotReady after the connection closed, the encoder's
    /// error with nothing sent (the default gives Unsupported for a non-default layout),
    /// TransportFailed when a line cannot be written. A failed write after the search already ended
    /// (its terminal event was delivered) is not an error: the call succeeds, so the caller sees
    /// one outcome.
    [[nodiscard]] std::expected<void, EngineError> startSearch(const Position& position,
                                                               const Limits&   limits,
                                                               EventSink&      sink) override;
    /// Sends YXSTOP if a search runs; the engine is expected to answer with a move, which ends it.
    /// Waits until a search script or command being written by another thread is complete.
    void stop() override;
    /// Writes the line to the engine, never inside a script being written. Errors:
    /// TransportFailed.
    [[nodiscard]] std::expected<void, EngineError> sendRaw(std::string_view line) override;

    /// Writes commandLines(command). None: done once written. Search: Busy until a terminal
    /// event, under the same ERROR rule as startSearch but with no illegal-move check (a command
    /// has no position). Until: Busy until `done` (called on the reader thread, outside the lock)
    /// accepts an event. A close ends a running command with ErrorEvent.
    /// Busy while a search or a Search/Until command runs, or while a command is written.
    /// With force over a running search or command: writes YXSTOP, queues the command and
    /// returns. The old terminal event reaches the old sink first; the command is written right
    /// after, on the reader thread (or on execute's thread if the move arrives while YXSTOP is
    /// written). startSearch and execute are Busy in between, also from the old sink's callback. A
    /// second force while one is queued is Busy. YXSTOP does not end an Until command whose engine
    /// is not thinking: the queued command waits for its end. A queued command dropped by a close,
    /// or whose write fails, gets one ErrorEvent on its sink (also for None, since execute already
    /// returned success). Rapfi V3 can drop a command sent right after its move, has no YXANALZ,
    /// and a one-shot reply has no terminator: Until can wait forever (docs/audit/AUD-08
    /// corrections). A V2 YXANALZ with no valid move answers only `ERROR`, which does not end a
    /// Search: use Until for it. Errors: Unsupported when canExecute is false (nothing sent),
    /// NotReady after the connection closed, Busy, TransportFailed as for startSearch. Every
    /// write (script, command, YXSTOP, sendRaw) holds one write lock, so YXSTOP never lands
    /// inside another thread's script; execute may wait for a script being written.
    [[nodiscard]] std::expected<void, EngineError> execute(const Command& command, EventSink& sink,
                                                           CommandOptions options = {}) override;

  private:
    void onLine(std::string_view line) override;
    void onClosed(CloseReason reason) override;

    // Swallows events while a None command is written; its address marks the engine Busy.
    struct DiscardSink final : EventSink {
        void onEvent(const SearchEvent&) override {}
    };
    // A command queued by execute(force), written after the running work's terminal event.
    struct Queued {
        std::vector<std::string> lines;
        Command::Completion      completion;
        EventSink*               sink;
        std::uint64_t            id;
        bool                     stopInFlight;  // execute is still writing YXSTOP
        bool                     active;        // claimed by a terminal event, now Busy as it
        std::uint64_t            activeId;      // searchId_ given to it when claimed
        bool                     delivered;     // the old terminal event reached the old sink
    };

    std::uint64_t beginCommand(const Command::Completion& completion, EventSink& sink);
    bool          finishLocked();
    void          flushQueued();
    EventSink*    failLocked();
    std::expected<void, EngineError> writeCommand(const std::vector<std::string>& lines,
                                                  std::uint64_t id, bool none, EventSink*& dropped);

    Transport&              transport_;
    parse::LineParser       parser_;
    RequestEncoder          encoder_;
    bool                    customEncoder_;
    std::mutex              mutex_;
    Rules                   rules_{};
    bool                    closed_{false};
    EventSink*              sink_{nullptr};  // non-null while a search or command runs
    std::optional<Position> position_;
    // counts started searches, so a failed start clears only its own
    std::uint64_t searchId_{0};
    // The running Until command's predicate; empty for a search or a Search command.
    std::function<bool(const SearchEvent&)> until_{};
    DiscardSink                             discard_{};
    std::optional<Queued>                   queued_{};
    std::uint64_t                           queueId_{0};
    // Held over every write (a search script, a command, YXSTOP, sendRaw), so no line lands
    // inside another thread's script. Lock order: writeMutex_, then mutex_. No sink is called
    // while either is held, and the reader thread takes writeMutex_ only to write a queued
    // command, holding nothing else; a holder never waits for the reader (beyond the transport's
    // own write buffer).
    std::mutex writeMutex_;
};

}  // namespace pygomo
