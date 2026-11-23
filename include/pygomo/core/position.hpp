#pragma once

#include <expected>
#include <span>
#include <vector>

#include "pygomo/core/board_layout.hpp"
#include "pygomo/core/coord.hpp"

namespace pygomo {

/// Why a Position could not be created or a move could not be played.
enum class PositionError {
    InvalidBoardSize,  ///< Width or height below 1 or above Position::kMaxSide.
    OutOfBounds,       ///< The point is not on the board.
    Occupied,          ///< The point already has a stone.
    Blocked            ///< The point is a blocked (WALL) cell of the layout.
};

/// A board of any rectangular size plus the ordered moves played on it.
/// Black moves first; the side to move alternates with each move.
class Position {
  public:
    /// Largest accepted width or height. A sanity bound, not a protocol limit.
    static constexpr int kMaxSide = 1024;

    /// Creates an empty board with the given layout. Errors: InvalidBoardSize outside
    /// 1..kMaxSide; OutOfBounds when a blocked cell or either cell of a portal is off the board.
    [[nodiscard]] static std::expected<Position, PositionError> create(int width, int height,
                                                                       BoardLayout layout = {});

    /// Number of columns.
    [[nodiscard]] int width() const noexcept { return width_; }
    /// Number of rows.
    [[nodiscard]] int height() const noexcept { return height_; }
    /// Black after an even number of moves, White after an odd one.
    [[nodiscard]] Side sideToMove() const noexcept;
    /// Moves in the order played. The view is invalidated by the next play().
    [[nodiscard]] std::span<const Move> moves() const noexcept { return moves_; }
    /// Structure of the board (torus, blocked cells, portals). Valid while the position lives.
    [[nodiscard]] const BoardLayout& layout() const noexcept { return layout_; }

    /// True if `c` lies on the board.
    [[nodiscard]] bool contains(Coord c) const noexcept;
    /// True if a stone stands on `c`. False for points outside the board.
    [[nodiscard]] bool occupied(Coord c) const noexcept;
    /// True if `c` is a blocked cell of the layout. False for points outside the board.
    [[nodiscard]] bool blocked(Coord c) const noexcept;

    /// Plays `c` for the side to move. On error the position is unchanged.
    /// Errors, checked in this order: OutOfBounds, Blocked, Occupied.
    [[nodiscard]] std::expected<void, PositionError> play(Coord c);

    /// Equal when the sizes and layouts match and the same moves were played in the same order.
    [[nodiscard]] bool operator==(const Position& other) const noexcept;

  private:
    Position(int width, int height, BoardLayout layout);

    [[nodiscard]] std::size_t index(Coord c) const noexcept;

    int               width_;
    int               height_;
    std::vector<Move> moves_;
    std::vector<bool> occupied_;
    BoardLayout       layout_;
    // Per-cell copy of layout_.blocked, so play() and blocked() need no search.
    std::vector<bool> blocked_;
};

}  // namespace pygomo
