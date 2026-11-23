#pragma once

#include <expected>
#include <functional>
#include <string>
#include <vector>

#include "pygomo/core/engine_port.hpp"
#include "pygomo/core/limits.hpp"
#include "pygomo/core/position.hpp"
#include "pygomo/core/rules.hpp"

namespace pygomo {

/// Everything one search asks of an engine: the stored rules plus the startSearch arguments.
struct SearchRequest {
    Rules    rules{};   ///< Rules set by setRules (the adapter's default if never set).
    Position position;  ///< Position to search from.
    Limits   limits{};  ///< Limits of this search.
};

/// The lines of one encoded search request, or why it cannot be sent.
using EncodedRequest = std::expected<std::vector<std::string>, EngineError>;

/// Turns one search request into the protocol lines that start it.
/// Returns every line of one startSearch, in order; an error (normally Unsupported) means
/// nothing is sent. It runs under the adapter lock: it must be pure, must not block and must
/// not call back into the adapter.
using RequestEncoder = std::function<EncodedRequest(const SearchRequest&)>;

}  // namespace pygomo
