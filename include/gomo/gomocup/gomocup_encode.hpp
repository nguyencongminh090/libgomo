#pragma once

// Gomocup line builders (adapter layer): the default search script and reusable blocks for
// custom RequestEncoders.

#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "gomo/core/search_request.hpp"

namespace gomo::encode {

/// Gomocup `INFO rule` id of `rules`, ignoring extensions. Empty when Gomocup has no id for the
/// board and opening rule pair (Standard or Renju with a swap opening).
[[nodiscard]] std::optional<int> gomocupRuleId(const Rules& rules);

/// Builds the default Gomocup script: START n (RECTSTART w,h for a non-square board), INFO rule,
/// INFO max_moves, timeout_turn, timeout_match, max_depth, max_node, then BEGIN on an empty board
/// or a BOARD..DONE block.
/// An engine without RECTSTART prints ERROR, which does not end a search (GOMO-04), so the
/// search stays open until a DeadlineEngine fires. Rapfi's FLIPY_X coordinate mode swaps w and
/// h; that mode is not handled here.
/// Errors: Unsupported for rules without a Gomocup rule id, non-empty rules.extensions or a
/// non-default position layout.
[[nodiscard]] std::expected<std::vector<std::string>, EngineError> defaultGomocupRequest(
    const SearchRequest& request);

/// Builds a position block: `command`, one `x,y,c` line per move, the `extra` lines as given
/// (e.g. walls `x,y,3`), then DONE. `c` is 1 for the side to move after these moves (the
/// engine's own stones; Black after an even count) and 2 for the other side.
[[nodiscard]] std::vector<std::string> boardBlock(std::string_view             command,
                                                  std::span<const Move>        moves,
                                                  std::span<const std::string> extra = {});

}  // namespace gomo::encode
