#include "gomo/parse/gtp_parser.hpp"

#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace gomo::parse {
namespace {

constexpr int kMaxLetterWidth = 625;  // 25 single + 25 * 24 double-letter columns, as the engine

bool isBlank(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

std::string_view trimRight(std::string_view s) {
    while (!s.empty() && isBlank(s.back())) s.remove_suffix(1);
    return s;
}

std::vector<std::string_view> tokens(std::string_view s) {
    std::vector<std::string_view> out;
    std::size_t                   i = 0;
    while (i < s.size()) {
        while (i < s.size() && isBlank(s[i])) ++i;
        std::size_t start = i;
        while (i < s.size() && !isBlank(s[i])) ++i;
        if (i > start) out.push_back(s.substr(start, i - start));
    }
    return out;
}

bool equalsIgnoreCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    return true;
}

template <class T> std::optional<T> toNumber(std::string_view s) {
    T v{};
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (s.empty() || ec != std::errc{} || p != s.data() + s.size()) return std::nullopt;
    return v;
}

constexpr std::string_view kColumns = "ABCDEFGHJKLMNOPQRSTUVWXYZ";  // no I

/// Column index 0..24 of one letter, or -1.
int letterIndex(char c) {
    auto up = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    auto at = kColumns.find(up);
    return at == std::string_view::npos ? -1 : static_cast<int>(at);
}

bool onBoard(Coord c, int width, int height) {
    return c.x >= 0 && c.y >= 0 && c.x < width && c.y < height;
}

bool isSkippedAnalyzeKey(std::string_view w) {
    return w == "scoreMean" || w == "scoreStdev" || w == "scoreLead" || w == "prior" ||
           w == "lcb" || w == "utilityLcb" || w == "weight" || w == "isSymmetryOf";
}

// Parses one `info` entry (tokens after `info`, before the next `info`); nullopt if malformed.
std::optional<Candidate> parseGtpAnalyzeEntry(std::span<const std::string_view> entry, int width,
                                              int height) {
    Candidate             cand;
    bool                  sawField   = false;
    bool                  hasUtility = false;
    std::optional<double> rawWinrate;

    for (std::size_t i = 0; i < entry.size(); ++i) {
        std::string_view w = entry[i];
        if (w == "move") {
            if (i + 1 >= entry.size()) return std::nullopt;
            auto vertex = parseGtpVertex(entry[++i], width, height);
            if (!vertex) return std::nullopt;
            if (auto* coord = std::get_if<Coord>(&*vertex)) {
                cand.move = *coord;
            } else if (std::holds_alternative<GtpPass>(*vertex)) {
                cand.move.reset();
            } else {
                return std::nullopt;
            }
            sawField = true;
        } else if (w == "visits") {
            if (i + 1 >= entry.size()) return std::nullopt;
            auto v = toNumber<std::int64_t>(entry[++i]);
            if (!v || *v < 0) return std::nullopt;
            cand.visits = *v;
            sawField    = true;
        } else if (w == "winrate") {
            if (i + 1 >= entry.size()) return std::nullopt;
            auto v = toNumber<double>(entry[++i]);
            if (!v) return std::nullopt;
            rawWinrate = *v;
            sawField   = true;
        } else if (w == "order") {
            if (i + 1 >= entry.size()) return std::nullopt;
            auto ord = toNumber<int>(entry[++i]);
            if (!ord || *ord < 0 || *ord == std::numeric_limits<int>::max()) {
                return std::nullopt;
            }
            cand.rank = *ord + 1;
            sawField  = true;
        } else if (w == "pv") {
            cand.principalVariation.clear();
            while (i + 1 < entry.size() && entry[i + 1] != "pvVisits" &&
                   entry[i + 1] != "pvEdgeVisits") {
                auto vertex = parseGtpVertex(entry[++i], width, height);
                if (!vertex) return std::nullopt;
                auto* coord = std::get_if<Coord>(&*vertex);
                if (!coord) break;
                cand.principalVariation.push_back(*coord);
            }
            sawField = true;
        } else if (w == "pvVisits" || w == "pvEdgeVisits") {
            while (i + 1 < entry.size() && toNumber<std::int64_t>(entry[i + 1])) ++i;
        } else if (w == "utility") {
            hasUtility = true;
            if (i + 1 < entry.size()) ++i;
        } else if (isSkippedAnalyzeKey(w)) {
            if (i + 1 < entry.size()) ++i;
        }
    }
    if (!sawField) return std::nullopt;
    if (rawWinrate) {
        double scaled = hasUtility ? *rawWinrate : *rawWinrate / 10000.0;
        if (std::isfinite(scaled) && scaled >= 0.0 && scaled <= 1.0) {
            cand.winrate = scaled == 0.0 ? 0.0 : scaled;
        }
    }
    return cand;
}

}  // namespace

std::optional<GtpResponseHeader> parseGtpResponseHeader(std::string_view line) {
    if (line.empty() || (line.front() != '=' && line.front() != '?')) return std::nullopt;
    GtpResponseHeader header;
    header.ok = line.front() == '=';
    line.remove_prefix(1);
    std::size_t digits = 0;
    while (digits < line.size() && line[digits] >= '0' && line[digits] <= '9') ++digits;
    if (digits > 9) return std::nullopt;
    if (digits > 0) {
        header.id = toNumber<int>(line.substr(0, digits));
        line.remove_prefix(digits);
    }
    // GTP: marker, optional id, then a space or the end of the line. `=== x ===` is free text.
    if (!line.empty() && !isBlank(line.front())) return std::nullopt;
    if (!line.empty() && line.front() == ' ') line.remove_prefix(1);
    header.body = std::string{trimRight(line)};
    return header;
}

std::optional<GtpVertex> parseGtpVertex(std::string_view text, int width, int height) {
    if (width <= 0 || height <= 0) return std::nullopt;
    while (!text.empty() && isBlank(text.front())) text.remove_prefix(1);
    text = trimRight(text);
    if (text.size() < 2) return std::nullopt;
    if (equalsIgnoreCase(text, "pass") || equalsIgnoreCase(text, "pss")) {
        return GtpVertex{GtpPass{}};
    }
    if (equalsIgnoreCase(text, "resign")) return GtpVertex{GtpResign{}};

    if (text.front() == '(') {
        if (text.back() != ')') return std::nullopt;
        text       = text.substr(1, text.size() - 2);
        auto comma = text.find(',');
        if (comma == std::string_view::npos) return std::nullopt;
        auto x = toNumber<int>(text.substr(0, comma));
        auto y = toNumber<int>(text.substr(comma + 1));
        if (!x || !y) return std::nullopt;
        Coord c{*x, *y};
        if (!onBoard(c, width, height)) return std::nullopt;
        return GtpVertex{c};
    }

    if (width > kMaxLetterWidth) return std::nullopt;
    int x = letterIndex(text[0]);
    if (x < 0) return std::nullopt;
    text.remove_prefix(1);
    if (std::isalpha(static_cast<unsigned char>(text.front()))) {
        int second = letterIndex(text.front());
        if (second < 0) return std::nullopt;
        x = (x + 1) * 25 + second;
        text.remove_prefix(1);
    }
    auto row = toNumber<int>(text);
    if (!row) return std::nullopt;
    Coord c{x, height - *row};
    if (!onBoard(c, width, height)) return std::nullopt;
    return GtpVertex{c};
}

std::optional<std::string> formatGtpVertex(Coord coord, int width, int height) {
    if (width <= 0 || height <= 0 || width > kMaxLetterWidth || !onBoard(coord, width, height))
        return std::nullopt;
    std::string out;
    if (coord.x <= 24) {
        out += kColumns[static_cast<std::size_t>(coord.x)];
    } else {
        out += kColumns[static_cast<std::size_t>(coord.x / 25 - 1)];
        out += kColumns[static_cast<std::size_t>(coord.x % 25)];
    }
    out += std::to_string(height - coord.y);
    return out;
}

SearchEvent parseGtpAnalyzeLine(std::string_view line, int width, int height) {
    auto unparsed = [&] { return SearchEvent{UnparsedEvent{std::string{line}}}; };
    auto words    = tokens(line);
    if (words.empty() || words.front() != "info") return unparsed();

    InfoEvent   info;
    std::size_t pos = 0;
    while (pos < words.size()) {
        std::size_t next = pos + 1;
        while (next < words.size() && words[next] != "info") ++next;
        std::span<const std::string_view> entry(words.data() + pos + 1, next - pos - 1);
        auto                              cand = parseGtpAnalyzeEntry(entry, width, height);
        if (!cand) {
            if (info.candidates.empty()) return unparsed();
            break;
        }
        info.candidates.push_back(std::move(*cand));
        pos = next;
    }

    const Candidate* summary   = nullptr;
    bool             anyRanked = false;
    for (const Candidate& c : info.candidates) {
        if (c.rank) {
            anyRanked = true;
            if (*c.rank == 1 && !summary) summary = &c;
        }
    }
    if (!anyRanked && !info.candidates.empty()) summary = &info.candidates.front();
    if (summary) info.principalVariation = summary->principalVariation;
    return info;
}

}  // namespace gomo::parse
