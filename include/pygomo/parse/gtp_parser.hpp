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

/// The vertex `pass`.
struct GtpPass {
    constexpr bool operator==(const GtpPass&) const = default;
};
/// The vertex `resign`.
struct GtpResign {
    constexpr bool operator==(const GtpResign&) const = default;
};
/// A parsed GTP vertex: a pass, a resignation or a board point (0-based, origin top-left).
using GtpVertex = std::variant<GtpPass, GtpResign, Coord>;

/// GTP vertex on a `width` x `height` board: `D4`, `j10`, `AA3` (columns skip I, rows count
/// from the bottom), the machine form `(x,y)` (0-based, origin top-left), `pass` or `resign`.
/// Returns nothing for anything else, including points outside the board.
[[nodiscard]] std::optional<GtpVertex> parseGtpVertex(std::string_view text, int width, int height);

/// Inverse of parseGtpVertex for a point; nothing when the point is outside the board or the
/// board is wider than 625 columns (the engine switches to the machine form there).
[[nodiscard]] std::optional<std::string> formatGtpVertex(Coord coord, int width, int height);

/// Parses one `info ...` line of `kata-analyze` or `lz-analyze` into an InfoEvent. Each `info`
/// entry becomes a Candidate in printed order (`move`, empty for pass; `visits`; `winrate`, 0..1
/// when `utility` is present else divided by 10000 and omitted outside 0..1; `order` + 1 as rank;
/// `pv`, stopping at pass, `pvVisits`, `pvEdgeVisits` or the next `info`). Summary `nodes` and
/// `score` stay empty; summary `principalVariation` comes from rank 1 (or the first entry when no
/// entry carries `order`). Score fields are ignored (KataGomo stores draw rate there). A non-info
/// line or broken first entry gives UnparsedEvent; a broken later entry keeps earlier candidates.
[[nodiscard]] SearchEvent parseGtpAnalyzeLine(std::string_view line, int width, int height);

}  // namespace pygomo::parse
