#pragma once

#include <cstdint>
#include <mutex>
#include <optional>

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
class KataGomoEngine final : public EnginePort, private TransportListener {
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
    /// See encode::defaultKataGomoRequest for default limitations.
    /// A failed write after the search already ended (its terminal event was delivered) is not
    /// an error: the call succeeds, so the caller sees one outcome.
    [[nodiscard]] std::expected<void, EngineError> startSearch(const Position& position,
                                                               const Limits&   limits,
                                                               EventSink&      sink) override;
    /// Does nothing: the engine cannot be interrupted during a search.
    void stop() override;
    /// Writes the line to the engine. Errors: TransportFailed.
    [[nodiscard]] std::expected<void, EngineError> sendRaw(std::string_view line) override;

  private:
    void onLine(std::string_view line) override;
    void onClosed(CloseReason reason) override;

    Transport&              transport_;
    parse::LineParser       parser_;
    bool                    customEncoder_;
    RequestEncoder          encoder_;
    std::mutex              mutex_;
    Rules                   rules_{};
    bool                    closed_{false};
    EventSink*              sink_{nullptr};  // non-null while a search runs
    std::optional<Position> position_;
    // counts started searches, so a failed start clears only its own
    std::uint64_t searchId_{0};
};

}  // namespace pygomo
