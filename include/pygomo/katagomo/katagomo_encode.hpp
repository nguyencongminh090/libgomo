#pragma once

#include "pygomo/core/search_request.hpp"

namespace pygomo::encode {

/// Builds the default KataGomo `gom` script with START, rule and limit settings, and BOARD..DONE.
/// Returns Unsupported for matchTime, maxDepth, swap openings, rule extensions, non-default
/// board layouts or rectangular boards: the `gom` mode only has `START n` (AUD-02, read from
/// the KataGomo source), so a non-square board cannot be sent.
[[nodiscard]] EncodedRequest defaultKataGomoRequest(const SearchRequest& request);

}  // namespace pygomo::encode
