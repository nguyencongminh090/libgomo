#pragma once

#include <expected>
#include <optional>

namespace pygomo {

/// Which lines win. Board rule and opening rule are separate on purpose: some engines merge
/// them into one number, but that is the adapter's problem, not the core's.
enum class BoardRule {
    Freestyle,  ///< Five or more in a row wins.
    Standard,   ///< Exactly five wins; an overline does not.
    Renju       ///< Black has forbidden moves (overline, double three, double four).
};

/// How the opening is played before normal alternation.
enum class OpeningRule {
    FreeOpen,  ///< No opening protocol.
    Swap1,     ///< Swap1 opening; the engine may answer with a swap.
    Swap2      ///< Swap2 opening; the engine may answer with a swap.
};

/// Why a Rules value is invalid.
enum class RulesError {
    InvalidMaxMoves  ///< maxMoves is set but not positive.
};

/// The rules of one game, as the caller asks for them. Whether an engine can play them is
/// EnginePort::supports().
struct Rules {
    BoardRule   board{BoardRule::Freestyle};     ///< Win condition.
    OpeningRule opening{OpeningRule::FreeOpen};  ///< Opening protocol.
    /// Game is a draw once this many moves are played. Empty means no limit.
    std::optional<int> maxMoves{};

    constexpr bool operator==(const Rules&) const = default;
};

/// Checks that `rules` is self-consistent. Errors: InvalidMaxMoves.
[[nodiscard]] constexpr std::expected<void, RulesError> validate(const Rules& rules) noexcept {
    if (rules.maxMoves && *rules.maxMoves <= 0) {
        return std::unexpected(RulesError::InvalidMaxMoves);
    }
    return {};
}

}  // namespace pygomo
