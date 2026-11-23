#include "gomo/parse/gomocup_parser.hpp"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace gomo::parse {
namespace {

bool startsWith(std::string_view s, std::string_view prefix) {
    return s.substr(0, prefix.size()) == prefix;
}

std::string_view trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) {
        s.remove_suffix(1);
    }
    return s;
}

std::vector<std::string_view> split(std::string_view s) {
    std::vector<std::string_view> out;
    while (true) {
        s = trim(s);
        if (s.empty()) return out;
        auto end = s.find_first_of(" \t");
        out.push_back(s.substr(0, end));
        if (end == std::string_view::npos) return out;
        s.remove_prefix(end);
    }
}

std::optional<int> toInt(std::string_view s) {
    int v{};
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{} || p != s.data() + s.size()) return std::nullopt;
    return v;
}

std::optional<int> toNonNegative(std::string_view s) {
    if (s.empty() || s.front() == '+' || s.front() == '-') return std::nullopt;
    auto v = toInt(s);
    return v && *v >= 0 ? v : std::nullopt;
}

/// "x,y" (0-based) or a label like "H8" (column letter, 1-based row; A1 is 0,0).
std::optional<Coord> toCoord(std::string_view s) {
    if (auto comma = s.find(','); comma != std::string_view::npos) {
        auto x = toNonNegative(s.substr(0, comma));
        auto y = toNonNegative(s.substr(comma + 1));
        if (x && y) return Coord{*x, *y};
        return std::nullopt;
    }
    if (s.size() >= 2 && s.front() >= 'A' && s.front() <= 'Z') {
        auto row = toNonNegative(s.substr(1));
        if (row && *row >= 1) return Coord{s.front() - 'A', *row - 1};
    }
    return std::nullopt;
}

/// Plain integer, or a decimal with a K / M / G suffix ("1.23M").
std::optional<std::int64_t> toCount(std::string_view s) {
    double scale = 1;
    if (!s.empty()) {
        switch (s.back()) {
        case 'K': scale = 1e3; break;
        case 'M': scale = 1e6; break;
        case 'G': scale = 1e9; break;
        case 'T': scale = 1e12; break;
        default: break;
        }
        if (scale != 1) s.remove_suffix(1);
    }
    double v{};
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (s.empty() || ec != std::errc{} || p != s.data() + s.size() || v < 0 || v * scale > 9e18) {
        return std::nullopt;
    }
    return static_cast<std::int64_t>(std::llround(v * scale));
}

/// Rapfi prints scores as a number or as +M<n> / -M<n> for a forced mate in n.
std::optional<Score> toScore(std::string_view s) {
    if (s.size() >= 3 && (s[0] == '+' || s[0] == '-') && s[1] == 'M') {
        auto n = toNonNegative(s.substr(2));
        if (!n) return std::nullopt;
        return Score{s[0] == '+' ? Score::Kind::MateIn : Score::Kind::MatedIn, *n};
    }
    if (auto v = toInt(s)) return Score{Score::Kind::Eval, *v};
    return std::nullopt;
}

/// "1ms", "12s", "3min", "2h" as Rapfi prints them (truncated, so only approximate).
std::optional<std::chrono::milliseconds> toDuration(std::string_view s) {
    std::int64_t unit = 0;
    for (auto [suffix, ms] : {std::pair<std::string_view, std::int64_t>{"ms", 1},
                              {"min", 60'000},
                              {"s", 1000},
                              {"h", 3'600'000}}) {
        if (s.size() > suffix.size() && s.substr(s.size() - suffix.size()) == suffix) {
            unit = ms;
            s.remove_suffix(suffix.size());
            break;
        }
    }
    auto n = unit ? toNonNegative(s) : std::nullopt;
    if (!n) return std::nullopt;
    return std::chrono::milliseconds{static_cast<std::int64_t>(*n) * unit};
}

std::vector<Coord> toPath(const std::vector<std::string_view>& tok, std::size_t from) {
    std::vector<Coord> pv;
    for (std::size_t i = from; i < tok.size(); ++i) {
        auto c = toCoord(tok[i]);
        if (!c) break;
        pv.push_back(*c);
    }
    return pv;
}

// "(N)" with N >= 1 at the start of Rapfi's normal MultiPV lines.
std::optional<int> toRankTag(std::string_view s) {
    if (s.size() < 3 || s.front() != '(' || s.back() != ')') return std::nullopt;
    auto r = toNonNegative(s.substr(1, s.size() - 2));
    return r && *r >= 1 ? r : std::nullopt;
}

// Percentage in 0..100 (e.g. "50.00") converted to 0..1; empty if invalid or out of range.
std::optional<double> toPercentRate(std::string_view s) {
    if (s.empty() || s.front() == '-') return std::nullopt;
    double v{};
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{} || p != s.data() + s.size() || !std::isfinite(v) || v < 0.0 ||
        v > 100.0) {
        return std::nullopt;
    }
    return v / 100.0;
}

// Reads "(W <pct>, D <pct>, ...)" from the tail of a ranked normal field into cand.
bool parseParenRates(std::string_view field, std::size_t from, Candidate& cand) {
    const auto open = field.find('(', from);
    if (open == std::string_view::npos) return false;
    const auto close = field.find(')', open + 1);
    if (close == std::string_view::npos) return false;

    std::string_view              body = field.substr(open + 1, close - open - 1);
    std::vector<std::string_view> tok;
    while (!body.empty()) {
        const auto start = body.find_first_not_of(" \t,");
        if (start == std::string_view::npos) break;
        body.remove_prefix(start);
        const auto end = body.find_first_of(" \t,");
        tok.push_back(body.substr(0, end));
        if (end == std::string_view::npos) break;
        body.remove_prefix(end);
    }

    bool any = false;
    for (std::size_t i = 0; i + 1 < tok.size(); i += 2) {
        if (tok[i] == "W") {
            if (auto w = toPercentRate(tok[i + 1])) {
                cand.winrate = w;
                any          = true;
            }
        } else if (tok[i] == "D") {
            if (auto d = toPercentRate(tok[i + 1])) {
                cand.drawrate = d;
                any           = true;
            }
        }
    }
    return any;
}

/// Rapfi's normal / brief message lines, separated by '|':
///   Speed 333K | Depth 5-6 | Eval -67 | Node 373 | Time 1ms
///   Depth 4-5 | Eval 0 | Time 1ms | E5 B2 A1 B1          (optional "[Pondering] " prefix)
///   Speed 12K | Visit 100 | Time 3ms                      (MCTS)
///   (1) 34 | 5-6 | E5 B2   and   (1) 34 (W 50.00, D 10.00, S 0.10) | V 1K | SD 12 | E5 B2
///   Bestline H7 J9 I9
/// Values are rounded the way the engine prints them.
std::optional<InfoEvent> parseNormalInfo(std::string_view text) {
    constexpr std::string_view pondering = "[Pondering] ";
    if (startsWith(text, pondering)) text.remove_prefix(pondering.size());

    InfoEvent info;
    bool      any = false;
    if (startsWith(text, "Bestline ")) {
        info.principalVariation = toPath(split(text.substr(9)), 0);
        if (info.principalVariation.empty()) return std::nullopt;
        return info;
    }

    std::vector<std::string_view> fields;
    while (true) {
        auto bar = text.find('|');
        fields.push_back(trim(text.substr(0, bar)));
        if (bar == std::string_view::npos) break;
        text.remove_prefix(bar + 1);
    }
    auto first = split(fields[0]);
    if (first.empty()) return std::nullopt;
    const auto rank = toRankTag(first[0]);
    if (!rank && first[0] != "Speed" && first[0] != "Depth") return std::nullopt;

    Candidate cand;
    if (rank) cand.rank = rank;

    for (std::size_t f = 0; f < fields.size(); ++f) {
        auto tok = split(fields[f]);
        if (tok.empty()) continue;
        if (rank && f == 0) {
            if (tok.size() >= 2) {
                if (auto sc = toScore(tok[1])) {
                    cand.score = sc;
                    any        = true;
                }
            }
            if (parseParenRates(fields[0], tok[0].size(), cand)) any = true;
        } else if (rank && f == 1 && tok.size() == 1 &&
                   tok[0].find(',') == std::string_view::npos && tok[0].front() >= '0' &&
                   tok[0].front() <= '9') {
            if (auto d = toNonNegative(tok[0].substr(0, tok[0].find('-')))) {
                info.depth = d;
                any        = true;
            }
        } else if (tok[0] == "Depth" && tok.size() >= 2) {
            if (auto d = toNonNegative(tok[1].substr(0, tok[1].find('-')))) {
                info.depth = d;
                any        = true;
            }
        } else if (tok[0] == "Eval" && tok.size() >= 2) {
            if (auto sc = toScore(tok[1])) {
                if (rank) cand.score = sc;
                else info.score = sc;
                any = true;
            }
        } else if ((tok[0] == "Node" || tok[0] == "Visit") && tok.size() >= 2) {
            if (auto n = toCount(tok[1])) {
                info.nodes = n;
                any        = true;
            }
        } else if (rank && tok[0] == "V" && tok.size() >= 2) {
            if (auto v = toCount(tok[1])) {
                cand.visits = v;
                any         = true;
            }
        } else if (tok[0] == "Time" && tok.size() >= 2) {
            if (auto t = toDuration(tok[1])) {
                info.elapsed = t;
                any          = true;
            }
        } else if (auto pv = toPath(tok, 0); !pv.empty() && pv.size() == tok.size()) {
            if (rank) {
                cand.move               = pv.front();
                cand.principalVariation = std::move(pv);
            } else {
                info.principalVariation = std::move(pv);
            }
            any = true;
        }
    }
    if (!any) return std::nullopt;
    if (rank) {
        if (*rank == 1) {
            info.score              = cand.score;
            info.principalVariation = cand.principalVariation;
        }
        info.candidates.push_back(std::move(cand));
    }
    return info;
}

/// `depth 10-12 ev 34 n 1.2M tm 900 pv 7,7 8,8`. The first token must be a known key (`depth` or
/// `multipv`) so that ordinary prose never turns into an info event. Unknown keys and their values
/// are skipped. Ranked lines (`multipv N`) produce one Candidate; only rank 1 fills summary
/// score/PV.
std::optional<InfoEvent> parseInfo(std::string_view text) {
    auto tok = split(text);
    if (tok.empty() || (tok[0] != "depth" && tok[0] != "multipv")) return std::nullopt;

    InfoEvent                   info;
    std::optional<int>          rank;
    std::optional<Score>        score;
    std::optional<std::int64_t> visits;
    std::optional<double>       winrate;
    std::optional<double>       drawrate;
    std::vector<Coord>          pv;

    for (std::size_t i = 0; i + 1 < tok.size(); i += 2) {
        auto key = tok[i];
        auto val = tok[i + 1];
        if (key == "pv") {
            pv = toPath(tok, i + 1);
            break;
        }
        if (key == "depth") {
            if (auto d = toNonNegative(val.substr(0, val.find('-')))) info.depth = d;
        } else if (key == "multipv") {
            auto r = toNonNegative(val);
            if (!r || *r < 1) return std::nullopt;
            rank = r;
        } else if (key == "ev") {
            if (auto e = toScore(val)) score = *e;
        } else if (key == "n") {
            if (auto n = toCount(val)) info.nodes = n;
        } else if (key == "v") {
            if (auto v = toCount(val)) visits = v;
        } else if (key == "w") {
            if (auto w = toPercentRate(val)) winrate = w;
        } else if (key == "d") {
            if (auto d = toPercentRate(val)) drawrate = d;
        } else if (key == "tm") {
            if (auto t = toNonNegative(val)) info.elapsed = std::chrono::milliseconds{*t};
        }
    }

    const bool hasSummary = info.depth || info.nodes || info.elapsed || score || !pv.empty();
    if (rank) {
        if (!hasSummary && !visits && !winrate && !drawrate) return std::nullopt;
        Candidate cand;
        cand.rank = rank;
        if (!pv.empty()) cand.move = pv.front();
        cand.score              = score;
        cand.visits             = visits;
        cand.winrate            = winrate;
        cand.drawrate           = drawrate;
        cand.principalVariation = std::move(pv);
        if (*rank == 1) {
            info.score              = cand.score;
            info.principalVariation = cand.principalVariation;
        }
        info.candidates.push_back(std::move(cand));
        return info;
    }
    if (!hasSummary) return std::nullopt;
    info.score              = score;
    info.principalVariation = std::move(pv);
    return info;
}

}  // namespace

SearchEvent parseGomocupLine(std::string_view rawLine) {
    const std::string_view line = trim(rawLine);
    const auto unparsed         = [&] { return SearchEvent{UnparsedEvent{std::string{rawLine}}}; };

    if (line == "SWAP") return SwapEvent{};
    if (startsWith(line, "ERROR")) {
        auto rest = line.substr(5);
        if (rest.empty() || rest.front() == ' ') return ErrorEvent{std::string{trim(rest)}};
        return unparsed();
    }
    if (startsWith(line, "MESSAGE ")) {
        if (auto info = parseInfo(line.substr(8))) return *info;
        if (auto info = parseNormalInfo(line.substr(8))) return *info;
        return SearchEvent{MessageEvent{std::string{line.substr(8)}}};
    }
    if (auto c = toCoord(line); c && line.find(',') != std::string_view::npos) {
        return BestMoveEvent{*c};
    }
    return unparsed();
}

}  // namespace gomo::parse
