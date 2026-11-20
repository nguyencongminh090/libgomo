#pragma once

namespace pygomo {

/// The two players. Black moves first.
enum class Side {
    Black, ///< Moves first.
    White ///< Moves second.
};

/// Returns the other side.
constexpr Side opposite(Side side) noexcept {
    return side == Side::Black ? Side::White : Side::Black;
}

}  // namespace pygomo
