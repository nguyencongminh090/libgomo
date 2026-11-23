#pragma once

#include "pygomo/core/search_request.hpp"

namespace pygomo::encode {

/// Builds the default KataGomo `gom` script with START, rule and limit settings, and BOARD..DONE.
/// Returns Unsupported for matchTime, maxDepth, swap openings, rule extensions, non-default
/// board layouts or rectangular boards. The `gom` command START takes a single board size.
[[nodiscard]] EncodedRequest defaultKataGomoRequest(const SearchRequest& request);

}  // namespace pygomo::encode
