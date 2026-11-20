#pragma once

#include <expected>
#include <string_view>

#include "pygomo/core/limits.hpp"
#include "pygomo/core/position.hpp"
#include "pygomo/core/rules.hpp"
#include "pygomo/core/search_event.hpp"

namespace pygomo {

enum class EngineError {
    Unsupported,       ///< The engine cannot express the request; nothing was sent.
    Busy,              ///< A search is already running.
    NotReady,          ///< The engine is not started or has gone away.
    TransportFailed,   ///< Writing to the engine failed.
};

/// Receives what an engine reports during a search. Called from the adapter's own thread.
class EventSink {
public:
    virtual ~EventSink() = default;
    virtual void onEvent(const SearchEvent& event) = 0;
};

/// The one interface applications use to talk to an engine, whatever protocol it speaks.
class EnginePort {
public:
    virtual ~EnginePort() = default;

    /// True if `rules` can be expressed in this engine's protocol.
    [[nodiscard]] virtual bool supports(const Rules& rules) const = 0;

    /// Unsupported combinations return Unsupported and send nothing.
    [[nodiscard]] virtual std::expected<void, EngineError> setRules(const Rules& rules) = 0;

    /// Starts a search and returns at once. Events go to `sink`, which must outlive the search.
    /// The search ends with exactly one terminal event: best move, swap, pass, resign or error.
    [[nodiscard]] virtual std::expected<void, EngineError> startSearch(
        const Position& position, const Limits& limits, EventSink& sink) = 0;

    /// Asks the engine to end the running search early. Harmless when none is running.
    virtual void stop() = 0;

    /// Sends a line as is. Replies arrive as events (usually MessageEvent or UnparsedEvent); nothing is promised.
    [[nodiscard]] virtual std::expected<void, EngineError> sendRaw(std::string_view line) = 0;
};

}  // namespace pygomo
