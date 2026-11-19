#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include "pygomo/core/coord.hpp"
#include "pygomo/core/search_event.hpp"

namespace pygomo::parse {

/// First line of a GTP response: `=<id> body` or `?<id> body`. Later body lines and the blank
/// terminator belong to the adapter. Every parser here is pure and total: it never throws.
struct GtpResponseHeader {
    bool ok{};                ///< `=` is true, `?` is false.
    std::optional<int> id{};  ///< The command id echoed by the engine, if any.
    std::string body{};       ///< Rest of the line, one leading space and trailing blanks removed.

    bool operator==(const GtpResponseHeader&) const = default;
};

/// Returns nothing when the line does not start with `=` or `?`, or the id has more than 9 digits.
[[nodiscard]] std::optional<GtpResponseHeader> parseGtpResponseHeader(std::string_view line);

struct GtpPass {
    constexpr bool operator==(const GtpPass&) const = default;
};
struct GtpResign {
    constexpr bool operator==(const GtpResign&) const = default;
};
using GtpVertex = std::variant<GtpPass, GtpResign, Coord>;

/// GTP vertex on a `width` x `height` board: `D4`, `j10`, `AA3` (columns skip I, rows count from the
/// bottom), the machine form `(x,y)` (0-based, origin top-left), `pass` or `resign`.
/// Returns nothing for anything else, including points outside the board.
[[nodiscard]] std::optional<GtpVertex> parseGtpVertex(std::string_view text, int width, int height);

/// Inverse of parseGtpVertex for a point; nothing when the point is outside the board or the board is
/// wider than 625 columns (the engine switches to the machine form there).
[[nodiscard]] std::optional<std::string> formatGtpVertex(Coord coord, int width, int height);

/// One `info ...` line of `kata-analyze` / `lz-analyze`. Only the first entry on the line (the engine's
/// best move) is read: `visits` becomes nodes and `pv` the principal variation, which stops at a pass.
/// Winrate, prior and the like are left out; in KataGomo Gom2026 the score fields hold the draw rate,
/// not a score. Anything else, or a bad PV vertex, comes back as UnparsedEvent with the whole line.
[[nodiscard]] SearchEvent parseGtpAnalyzeLine(std::string_view line, int width, int height);

}  // namespace pygomo::parse
