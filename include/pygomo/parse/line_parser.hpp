#pragma once

#include <functional>
#include <string_view>
#include <utility>

#include "pygomo/core/search_event.hpp"

namespace pygomo::parse {

/// Turns one engine line into an event. An adapter calls it on the transport's reader thread,
/// never under its own lock, so it must not block. It must not turn a line that ends a search
/// (a move, SWAP, ERROR in a protocol where that is fatal) into a non-terminal event: the
/// adapter trusts `isTerminal` of the result and cannot tell the line was a move.
using LineParser = std::function<SearchEvent(std::string_view)>;

/// A parser that asks `custom` first and falls back to `fallback` when `custom` answers
/// UnparsedEvent. Both are copied; either may be empty (an empty `custom` is skipped).
[[nodiscard]] inline LineParser chainParsers(LineParser custom, LineParser fallback) {
    return [custom = std::move(custom), fallback = std::move(fallback)](std::string_view line) {
        if (custom) {
            SearchEvent event = custom(line);
            if (!std::holds_alternative<UnparsedEvent>(event)) return event;
        }
        return fallback(line);
    };
}

}  // namespace pygomo::parse
