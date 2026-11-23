#pragma once

#include "gomo/core/search_request.hpp"

namespace gomo::encode {

/// Builds the default KataGomo `gom` script with START, rule and limit settings, and BOARD..DONE.
/// Returns Unsupported for matchTime, maxDepth, swap openings, rule extensions, non-default
/// board layouts or rectangular boards: the `gom` mode only has `START n` (AUD-02, read from
/// the KataGomo source), so a non-square board cannot be sent.
[[nodiscard]] EncodedRequest defaultKataGomoRequest(const SearchRequest& request);

}  // namespace gomo::encode
