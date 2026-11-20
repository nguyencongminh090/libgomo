#include "pygomo/katagomo/katagomo_engine.hpp"

#include <string>
#include <vector>

#include "pygomo/parse/gomocup_parser.hpp"

namespace pygomo {
namespace {

const char* ruleCommand(BoardRule r) {
    switch (r) {
        case BoardRule::Freestyle: return "FREESTYLE";
        case BoardRule::Standard: return "STANDARD";
        case BoardRule::Renju: return "RENJU";
    }
    return "FREESTYLE";
}

}  // namespace

KataGomoEngine::KataGomoEngine(Transport& transport) : transport_(transport) {}

std::expected<void, EngineError> KataGomoEngine::open() {
    if (!transport_.start(*this)) return std::unexpected(EngineError::NotReady);
    return {};
}

bool KataGomoEngine::supports(const Rules& rules) const {
    return rules.opening == OpeningRule::FreeOpen && validate(rules).has_value();
}

std::expected<void, EngineError> KataGomoEngine::setRules(const Rules& rules) {
    if (!supports(rules)) return std::unexpected(EngineError::Unsupported);
    std::lock_guard lock(mutex_);
    rules_ = rules;
    return {};
}

std::expected<void, EngineError> KataGomoEngine::startSearch(
    const Position& position, const Limits& limits, EventSink& sink) {
    // Nothing is sent for a request the protocol cannot express.
    if (limits.matchTime || limits.maxDepth) return std::unexpected(EngineError::Unsupported);
    if (position.width() != position.height()) return std::unexpected(EngineError::Unsupported);

    std::vector<std::string> lines;
    {
        std::lock_guard lock(mutex_);
        if (closed_) return std::unexpected(EngineError::NotReady);
        if (sink_) return std::unexpected(EngineError::Busy);
        // START resets the board, so the settings follow it.
        lines.push_back("START " + std::to_string(position.width()));
        lines.push_back(ruleCommand(rules_.board));
        if (rules_.maxMoves) lines.push_back("maxmoves " + std::to_string(*rules_.maxMoves));
        if (limits.turnTime) lines.push_back("INFO timeout_turn " + std::to_string(limits.turnTime->count()));
        if (limits.maxNodes) lines.push_back("kata-set-param maxVisits " + std::to_string(*limits.maxNodes));
        // BOARD alternates colours from Black and ignores any third field; an empty list is the empty board.
        lines.push_back("BOARD");
        for (const Move& m : position.moves()) {
            lines.push_back(std::to_string(m.coord.x) + "," + std::to_string(m.coord.y));
        }
        lines.push_back("DONE");
        sink_ = &sink;
        position_ = position;
    }
    for (const auto& l : lines) {
        if (!transport_.writeLine(l)) {
            std::lock_guard lock(mutex_);
            sink_ = nullptr;
            return std::unexpected(EngineError::TransportFailed);
        }
    }
    return {};
}

void KataGomoEngine::stop() {}

std::expected<void, EngineError> KataGomoEngine::sendRaw(std::string_view line) {
    if (!transport_.writeLine(line)) return std::unexpected(EngineError::TransportFailed);
    return {};
}

void KataGomoEngine::onLine(std::string_view line) {
    if (line == "OK") return;  // reply to START, not an event
    SearchEvent event = parse::parseGomocupLine(line);
    EventSink* sink = nullptr;
    {
        std::lock_guard lock(mutex_);
        sink = sink_;
        if (!sink) return;  // banner or reply outside a search
        if (auto* best = std::get_if<BestMoveEvent>(&event)) {
            if (!position_->contains(best->coord) || position_->occupied(best->coord)) {
                event = ErrorEvent{"engine returned an illegal move: " + std::string{line}};
            }
        }
        const bool terminal = !std::holds_alternative<InfoEvent>(event) &&
                              !std::holds_alternative<UnparsedEvent>(event);
        if (terminal) sink_ = nullptr;
    }
    sink->onEvent(event);
}

void KataGomoEngine::onClosed(CloseReason) {
    EventSink* sink = nullptr;
    {
        std::lock_guard lock(mutex_);
        closed_ = true;
        sink = sink_;
        sink_ = nullptr;
    }
    if (sink) sink->onEvent(ErrorEvent{"engine connection closed"});
}

}  // namespace pygomo
