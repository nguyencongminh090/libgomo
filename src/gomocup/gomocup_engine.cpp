#include "pygomo/gomocup/gomocup_engine.hpp"

#include <string>

#include "pygomo/parse/gomocup_parser.hpp"

namespace pygomo {
namespace {

std::optional<int> ruleId(const Rules& r) {
    if (r.opening == OpeningRule::FreeOpen) {
        switch (r.board) {
            case BoardRule::Freestyle: return 0;
            case BoardRule::Standard: return 1;
            case BoardRule::Renju: return 4;
        }
    }
    if (r.board == BoardRule::Freestyle) {
        return r.opening == OpeningRule::Swap1 ? 5 : 6;
    }
    return std::nullopt;
}

}  // namespace

GomocupEngine::GomocupEngine(Transport& transport) : transport_(transport) {}

std::expected<void, EngineError> GomocupEngine::open() {
    if (!transport_.start(*this)) return std::unexpected(EngineError::NotReady);
    return {};
}

bool GomocupEngine::supports(const Rules& rules) const {
    return ruleId(rules).has_value() && validate(rules).has_value();
}

std::expected<void, EngineError> GomocupEngine::setRules(const Rules& rules) {
    if (!supports(rules)) return std::unexpected(EngineError::Unsupported);
    std::lock_guard lock(mutex_);
    rules_ = rules;
    return {};
}

std::expected<void, EngineError> GomocupEngine::startSearch(
    const Position& position, const Limits& limits, EventSink& sink) {
    if (position.width() != position.height()) return std::unexpected(EngineError::Unsupported);

    std::vector<std::string> lines;
    {
        std::lock_guard lock(mutex_);
        if (closed_) return std::unexpected(EngineError::NotReady);
        if (sink_) return std::unexpected(EngineError::Busy);
        lines.push_back("START " + std::to_string(position.width()));
        lines.push_back("INFO RULE " + std::to_string(*ruleId(rules_)));
        if (rules_.maxMoves) lines.push_back("INFO MAX_MOVES " + std::to_string(*rules_.maxMoves));
        if (limits.turnTime) lines.push_back("INFO TIMEOUT_TURN " + std::to_string(limits.turnTime->count()));
        if (limits.matchTime) lines.push_back("INFO TIMEOUT_MATCH " + std::to_string(limits.matchTime->count()));
        if (limits.maxDepth) lines.push_back("INFO MAX_DEPTH " + std::to_string(*limits.maxDepth));
        if (limits.maxNodes) lines.push_back("INFO MAX_NODE " + std::to_string(*limits.maxNodes));
        if (position.moves().empty()) {
            lines.push_back("BEGIN");
        } else {
            lines.push_back("BOARD");
            const Side mine = position.sideToMove();
            for (const Move& m : position.moves()) {
                lines.push_back(std::to_string(m.coord.x) + "," + std::to_string(m.coord.y) + "," +
                                (m.side == mine ? "1" : "2"));
            }
            lines.push_back("DONE");
        }
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

void GomocupEngine::stop() {
    {
        std::lock_guard lock(mutex_);
        if (!sink_) return;
    }
    (void)transport_.writeLine("YXSTOP");
}

std::expected<void, EngineError> GomocupEngine::sendRaw(std::string_view line) {
    if (!transport_.writeLine(line)) return std::unexpected(EngineError::TransportFailed);
    return {};
}

void GomocupEngine::onLine(std::string_view line) {
    SearchEvent event = parse::parseGomocupLine(line);
    EventSink* sink = nullptr;
    {
        std::lock_guard lock(mutex_);
        sink = sink_;
        if (!sink) return;  // engine banner or reply outside a search
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

void GomocupEngine::onClosed(CloseReason) {
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
