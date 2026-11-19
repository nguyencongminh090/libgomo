#pragma once

#include <expected>
#include <span>
#include <vector>

#include "pygomo/core/coord.hpp"

namespace pygomo {

enum class PositionError { InvalidBoardSize, OutOfBounds, Occupied };

/// A board of any rectangular size plus the ordered moves played on it.
/// Black moves first; the side to move alternates with each move.
class Position {
public:
    /// Largest accepted width or height. A sanity bound, not a protocol limit.
    static constexpr int kMaxSide = 1024;

    [[nodiscard]] static std::expected<Position, PositionError> create(int width, int height);

    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] Side sideToMove() const noexcept;
    [[nodiscard]] std::span<const Move> moves() const noexcept { return moves_; }

    [[nodiscard]] bool contains(Coord c) const noexcept;
    /// False for points outside the board.
    [[nodiscard]] bool occupied(Coord c) const noexcept;

    /// Plays `c` for the side to move. On error the position is unchanged.
    [[nodiscard]] std::expected<void, PositionError> play(Coord c);

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
