#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

namespace pygomo {

/// Search limits for one request. An empty field means "no limit from the caller";
/// the engine then applies its own default.
struct Limits {
    std::optional<std::chrono::milliseconds> turnTime{};
    std::optional<std::chrono::milliseconds> matchTime{};
    std::optional<int> maxDepth{};
    std::optional<std::int64_t> maxNodes{};

    constexpr bool operator==(const Limits&) const = default;
};

}  // namespace pygomo
