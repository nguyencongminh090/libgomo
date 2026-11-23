#pragma once

#include <chrono>
#include <optional>

#include "pygomo/core/search_request.hpp"

namespace pygomo::encode {

/// Builds the default GTP search commands without ids.
/// The last command is `genmove` by default. An analyze interval selects `kata-genmove_analyze`.
/// The interval is rounded down to whole centiseconds, with a minimum of one centisecond.
/// Returns Unsupported for matchTime, maxDepth, swap openings, rule extensions, non-default
/// board layouts or moves without a GTP vertex.
[[nodiscard]] EncodedRequest defaultGtpRequest(
    const SearchRequest& request, std::optional<std::chrono::milliseconds> analyzeInterval);

}  // namespace pygomo::encode
