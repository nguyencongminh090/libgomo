#include "pygomo/parse/gomocup_parser.hpp"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pygomo::parse {
namespace {

bool startsWith(std::string_view s, std::string_view prefix) {
    return s.substr(0, prefix.size()) == prefix;
}

std::string_view trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) s.remove_suffix(1);
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

/// `depth 10-12 ev 34 n 1.2M tm 900 pv 7,7 8,8`. The first token must be a known key so that
/// ordinary prose never turns into an info event. Unknown keys and their values are skipped.
std::optional<InfoEvent> parseInfo(std::string_view text) {
    auto tok = split(text);
    if (tok.empty() || (tok[0] != "depth" && tok[0] != "multipv")) return std::nullopt;

    InfoEvent info;
    bool any = false;
    for (std::size_t i = 0; i + 1 < tok.size(); i += 2) {
        auto key = tok[i];
        auto val = tok[i + 1];
        if (key == "pv") {
            for (std::size_t j = i + 1; j < tok.size(); ++j) {
                auto c = toCoord(tok[j]);
                if (!c) break;
                info.principalVariation.push_back(*c);
            }
            any = any || !info.principalVariation.empty();
            break;
        }
        if (key == "depth") {
            if (auto d = toNonNegative(val.substr(0, val.find('-')))) { info.depth = d; any = true; }
        } else if (key == "ev") {
            if (auto e = toInt(val)) { info.score = Score{Score::Kind::Eval, *e}; any = true; }
        } else if (key == "n") {
            if (auto n = toCount(val)) { info.nodes = n; any = true; }
        } else if (key == "tm") {
            if (auto t = toNonNegative(val)) { info.elapsed = std::chrono::milliseconds{*t}; any = true; }
        }
    }
    if (!any) return std::nullopt;
    return info;
}

}  // namespace

SearchEvent parseGomocupLine(std::string_view rawLine) {
    const std::string_view line = trim(rawLine);
    const auto unparsed = [&] { return SearchEvent{UnparsedEvent{std::string{rawLine}}}; };

    if (line == "SWAP") return SwapEvent{};
    if (startsWith(line, "ERROR")) {
        auto rest = line.substr(5);
        if (rest.empty() || rest.front() == ' ') return ErrorEvent{std::string{trim(rest)}};
        return unparsed();
    }
    if (startsWith(line, "MESSAGE ")) {
        if (auto info = parseInfo(line.substr(8))) return *info;
        return unparsed();
    }
    if (auto c = toCoord(line); c && line.find(',') != std::string_view::npos) {
        return BestMoveEvent{*c};
    }
    return unparsed();
}

}  // namespace pygomo::parse
