#include "pygomo/parse/gtp_parser.hpp"

#include <cctype>
#include <charconv>
#include <cstdint>
#include <vector>

namespace pygomo::parse {
namespace {

constexpr int kMaxLetterWidth = 625;  // 25 single + 25 * 24 double-letter columns, as the engine

bool isBlank(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

std::string_view trimRight(std::string_view s) {
    while (!s.empty() && isBlank(s.back())) s.remove_suffix(1);
    return s;
}

std::vector<std::string_view> tokens(std::string_view s) {
    std::vector<std::string_view> out;
    std::size_t i = 0;
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

template <class T>
std::optional<T> toNumber(std::string_view s) {
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
        text = text.substr(1, text.size() - 2);
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
    auto words = tokens(line);
    if (words.empty() || words.front() != "info") return unparsed();

    InfoEvent info;
    bool sawField = false;
    for (std::size_t i = 1; i < words.size(); ++i) {
        std::string_view w = words[i];
        if (w == "info") break;  // second entry: only the first is read
        if (w == "visits" && i + 1 < words.size()) {
            auto v = toNumber<std::int64_t>(words[++i]);
            if (!v) return unparsed();
            info.nodes = *v;
            sawField = true;
        } else if (w == "pv") {
            while (i + 1 < words.size() && words[i + 1] != "pvVisits" &&
                   words[i + 1] != "pvEdgeVisits" && words[i + 1] != "info") {
                auto vertex = parseGtpVertex(words[++i], width, height);
                if (!vertex) return unparsed();
                auto* coord = std::get_if<Coord>(&*vertex);
                if (!coord) break;
                info.principalVariation.push_back(*coord);
            }
            sawField = true;
        }
    }
    if (!sawField) return unparsed();
    return info;
}

}  // namespace pygomo::parse
