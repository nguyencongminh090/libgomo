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

/// EnginePort for KataGomo's `gom` mode (gomprotocol.cpp): GTP-style rule commands plus Gomocup
/// START / BOARD..DONE. Derived from reading the source; no real engine has been run.
/// The transport is not owned and must outlive the engine. Call open() once before use.
///
/// The engine reads one line at a time, so stop() cannot interrupt a search and is a no-op.
/// The sink is called from the transport's reader thread, never while the engine holds a lock.
/// A search ends with a move, an illegal move or `ERROR` (ErrorEvent) or a closed connection.
/// The engine has no deadline of its own; see DeadlineEngine.
class KataGomoEngine final : public EnginePort, public CommandPort, private TransportListener {
  public:
    /// Binds the engine to `transport`; nothing is read or sent until open().
    /// `parser` reads every engine line (except `OK`) on the reader thread; empty means
    /// parseGomocupLine. To add to the default instead of replacing it, pass
    /// chainParsers(custom, parseGomocupLine). The illegal-move check still runs on the result.
    /// `encoder` builds the search script under the engine lock. It must be pure and must not
    /// call back into the engine. An empty encoder uses encode::defaultKataGomoRequest.
    explicit KataGomoEngine(Transport& transport, parse::LineParser parser = {},
                            RequestEncoder encoder = {});

    /// Starts reading from the transport. Returns NotReady if the transport cannot start.
    [[nodiscard]] std::expected<void, EngineError> open();

    /// Accepts valid rules with free opening and empty extensions by default.
    /// With a custom encoder, accepts any rules that pass validate().
    [[nodiscard]] bool supports(const Rules& rules) const override;
    /// Stores the rules for later searches; sends nothing. Unsupported if supports() is false.
    [[nodiscard]] std::expected<void, EngineError> setRules(const Rules& rules) override;
    /// Sends the encoded lines, by default START, the rule, limit settings and BOARD..DONE.
    /// Returns Busy while a search runs, NotReady after close or TransportFailed on write failure.
    /// An encoding error is returned before any writes. An empty script returns Unsupported.
    /// See encode::defaultKataGomoRequest for default limitations. A non-square board stays
    /// Unsupported: `gom` mode only has START n (AUD-02).
    /// A failed write after the search already ended (its terminal event was delivered) is not
    /// an error: the call succeeds, so the caller sees one outcome.
    [[nodiscard]] std::expected<void, EngineError> startSearch(const Position& position,
                                                               const Limits&   limits,
                                                               EventSink&      sink) override;
    /// Does nothing: the engine cannot be interrupted during a search.
    void stop() override;
    /// Writes the line to the engine. Errors: TransportFailed.
    [[nodiscard]] std::expected<void, EngineError> sendRaw(std::string_view line) override;

    /// Writes commandLines(command). None: done once written. Search: Busy until a terminal
    /// event (an `ERROR` line ends it, as for startSearch), with no illegal-move check (a command
    /// has no position). Until: Busy until `done` (called on the reader thread, outside the lock)
    /// accepts an event. A close ends a running command with ErrorEvent.
    /// Busy while a search or a Search/Until command runs, or while a command is written.
    /// With force over a running search or command: stop() cannot interrupt it, so the command is
    /// queued and execute returns; it is written from the reader thread right after the natural
    /// terminal event has reached the old sink (startSearch and execute are Busy in between).
    /// Wrap in DeadlineEngine to bound the wait. A second force while one is queued is Busy.
    /// A queued command dropped by a close, or whose write fails, gets one ErrorEvent on its sink
    /// (also for None, since execute already returned success).
    /// Errors: Unsupported for an empty command or an Until without `done`, NotReady after the
    /// connection closed, Busy, TransportFailed as for startSearch.
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
        bool                     active;    // claimed by a terminal event, now Busy as it
        std::uint64_t            activeId;  // searchId_ given to it when claimed
    };

    std::uint64_t beginCommand(const Command::Completion& completion, EventSink& sink);
    bool          finishLocked();
    void          flushQueued();
    std::expected<void, EngineError> writeCommand(const std::vector<std::string>& lines,
                                                  std::uint64_t id, bool none);

    Transport&              transport_;
    parse::LineParser       parser_;
    bool                    customEncoder_;
    RequestEncoder          encoder_;
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
};

}  // namespace pygomo
