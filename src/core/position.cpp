#include "pygomo/core/position.hpp"

#include <cstddef>

namespace pygomo {

Position::Position(int width, int height)
    : width_(width),
      height_(height),
      occupied_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), false) {}

std::expected<Position, PositionError> Position::create(int width, int height) {
    if (width < 1 || height < 1 || width > kMaxSide || height > kMaxSide) {
        return std::unexpected(PositionError::InvalidBoardSize);
    }
    return Position(width, height);
}

Side Position::sideToMove() const noexcept {
    return moves_.size() % 2 == 0 ? Side::Black : Side::White;
}

bool Position::contains(Coord c) const noexcept {
    return c.x >= 0 && c.x < width_ && c.y >= 0 && c.y < height_;
}

std::size_t Position::index(Coord c) const noexcept {
    return static_cast<std::size_t>(c.y) * static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(c.x);
}

bool Position::occupied(Coord c) const noexcept {
    return contains(c) && occupied_[index(c)];
}

std::expected<void, PositionError> Position::play(Coord c) {
    if (!contains(c)) return std::unexpected(PositionError::OutOfBounds);
    if (occupied_[index(c)]) return std::unexpected(PositionError::Occupied);
    moves_.push_back(Move{c, sideToMove()});
    occupied_[index(c)] = true;
    return {};
}

bool Position::operator==(const Position& other) const noexcept {
    return width_ == other.width_ && height_ == other.height_ && moves_ == other.moves_;
}

}  // namespace pygomo
