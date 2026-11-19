#pragma once

namespace pygomo {

enum class Side { Black, White };

constexpr Side opposite(Side side) noexcept {
    return side == Side::Black ? Side::White : Side::Black;
}

}  // namespace pygomo
