#include "gomo/gtp/gtp_encode.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "gomo/parse/gtp_parser.hpp"

namespace gomo::encode {
namespace {

const char* basicRule(BoardRule r) {
    switch (r) {
    case BoardRule::Freestyle: return "FREESTYLE";
    case BoardRule::Standard: return "STANDARD";
    case BoardRule::Renju: return "RENJU";
    }
    return nullptr;
}

const char* color(Side s) { return s == Side::Black ? "B" : "W"; }

}  // namespace

EncodedRequest defaultGtpRequest(const SearchRequest&                     request,
                                 std::optional<std::chrono::milliseconds> analyzeInterval) {
    const Rules&    rules    = request.rules;
    const Position& position = request.position;
    const Limits&   limits   = request.limits;
    const auto*     rule     = basicRule(rules.board);
    if (!rule || rules.opening != OpeningRule::FreeOpen || !rules.extensions.empty() ||
        position.layout() != BoardLayout{} || limits.matchTime || limits.maxDepth) {
        return std::unexpected(EngineError::Unsupported);
    }

    std::vector<std::string> commands;
    commands.push_back(std::format("boardsize {} {}", position.width(), position.height()));
    commands.push_back("clear_board");
    commands.push_back(rule);
    if (rules.maxMoves) commands.push_back(std::format("maxmoves {}", *rules.maxMoves));
    if (limits.turnTime) {
        const auto ms = limits.turnTime->count();
        commands.push_back(std::format("kata-set-param maxTime {}.{:03}", ms / 1000, ms % 1000));
    }
    if (limits.maxNodes) {
        commands.push_back(std::format("kata-set-param maxVisits {}", *limits.maxNodes));
    }
    for (const Move& m : position.moves()) {
        auto vertex = parse::formatGtpVertex(m.coord, position.width(), position.height());
        if (!vertex) return std::unexpected(EngineError::Unsupported);
        commands.push_back(std::format("play {} {}", color(m.side), *vertex));
    }
    if (analyzeInterval) {
        const auto cs = std::max<std::int64_t>(1, analyzeInterval->count() / 10);
        commands.push_back(
            std::format("kata-genmove_analyze {} interval {}", color(position.sideToMove()), cs));
    } else {
        commands.push_back(std::format("genmove {}", color(position.sideToMove())));
    }
    return commands;
}

}  // namespace gomo::encode
