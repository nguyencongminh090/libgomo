#pragma once

#include <expected>
#include <optional>

namespace pygomo {

/// Board rule and opening rule are separate on purpose: some engines merge them
/// into one number, but that is the adapter's problem, not the core's.
enum class BoardRule { Freestyle, Standard, Renju };
enum class OpeningRule { FreeOpen, Swap1, Swap2 };

enum class RulesError { InvalidMaxMoves };

struct Rules {
    BoardRule board{BoardRule::Freestyle};
    OpeningRule opening{OpeningRule::FreeOpen};
    /// Game is a draw once this many moves are played. Empty means no limit.
    std::optional<int> maxMoves{};

    constexpr bool operator==(const Rules&) const = default;
};

[[nodiscard]] constexpr std::expected<void, RulesError> validate(const Rules& rules) noexcept {
    if (rules.maxMoves && *rules.maxMoves <= 0) {
        return std::unexpected(RulesError::InvalidMaxMoves);
    }
    return {};
}

}  // namespace pygomo
