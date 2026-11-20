#pragma once

#include <expected>
#include <span>
#include <vector>

#include "pygomo/core/coord.hpp"

namespace pygomo {

/// Why a Position could not be created or a move could not be played.
enum class PositionError {
    InvalidBoardSize, ///< Width or height below 1 or above Position::kMaxSide.
    OutOfBounds, ///< The point is not on the board.
    Occupied ///< The point already has a stone.
};

/// A board of any rectangular size plus the ordered moves played on it.
/// Black moves first; the side to move alternates with each move.
class Position {
public:
    /// Largest accepted width or height. A sanity bound, not a protocol limit.
    static constexpr int kMaxSide = 1024;

    /// Creates an empty board. Errors: InvalidBoardSize outside 1..kMaxSide.
    [[nodiscard]] static std::expected<Position, PositionError> create(int width, int height);

    /// Number of columns.
    [[nodiscard]] int width() const noexcept { return width_; }
    /// Number of rows.
    [[nodiscard]] int height() const noexcept { return height_; }
    /// Black after an even number of moves, White after an odd one.
    [[nodiscard]] Side sideToMove() const noexcept;
    /// Moves in the order played. The view is invalidated by the next play().
    [[nodiscard]] std::span<const Move> moves() const noexcept { return moves_; }

    /// True if `c` lies on the board.
    [[nodiscard]] bool contains(Coord c) const noexcept;
    /// True if a stone stands on `c`. False for points outside the board.
    [[nodiscard]] bool occupied(Coord c) const noexcept;

    /// Plays `c` for the side to move. On error the position is unchanged.
    /// Errors: OutOfBounds, Occupied.
    [[nodiscard]] std::expected<void, PositionError> play(Coord c);

    /// Equal when the sizes match and the same moves were played in the same order.
    [[nodiscard]] bool operator==(const Position& other) const noexcept;

private:
    Position(int width, int height);

    [[nodiscard]] std::size_t index(Coord c) const noexcept;

    int width_;
    int height_;
    std::vector<Move> moves_;
    std::vector<bool> occupied_;
};

}  // namespace pygomo
