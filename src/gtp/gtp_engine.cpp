#include "pygomo/gtp/gtp_engine.hpp"

#include <format>
#include <vector>

#include "pygomo/parse/gtp_parser.hpp"

namespace pygomo {
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

GtpEngine::GtpEngine(Transport& transport) : transport_(transport) {}

std::expected<void, EngineError> GtpEngine::open() {
    if (!transport_.start(*this)) return std::unexpected(EngineError::NotReady);
    return {};
}

bool GtpEngine::supports(const Rules& rules) const {
    return rules.opening == OpeningRule::FreeOpen && validate(rules).has_value();
}

std::expected<void, EngineError> GtpEngine::setRules(const Rules& rules) {
    if (!supports(rules)) return std::unexpected(EngineError::Unsupported);
    std::lock_guard lock(mutex_);
    rules_ = rules;
    return {};
}

std::expected<void, EngineError> GtpEngine::startSearch(
    const Position& position, const Limits& limits, EventSink& sink) {
    // Nothing is sent for a request the protocol cannot express.
    if (limits.matchTime || limits.maxDepth) return std::unexpected(EngineError::Unsupported);
    std::vector<std::string> commands;
    for (const Move& m : position.moves()) {
        auto vertex = parse::formatGtpVertex(m.coord, position.width(), position.height());
        if (!vertex) return std::unexpected(EngineError::Unsupported);
        commands.push_back(std::format("play {} {}", color(m.side), *vertex));
    }

    std::vector<std::string> lines;
    {
        std::lock_guard lock(mutex_);
        if (closed_) return std::unexpected(EngineError::NotReady);
        if (sink_) return std::unexpected(EngineError::Busy);

        std::vector<std::string> all;
        all.push_back(std::format("boardsize {} {}", position.width(), position.height()));
        all.push_back("clear_board");
        all.push_back(basicRule(rules_.board));
        if (rules_.maxMoves) all.push_back(std::format("maxmoves {}", *rules_.maxMoves));
        if (limits.turnTime) {
            const auto ms = limits.turnTime->count();
            all.push_back(std::format("kata-set-param maxTime {}.{:03}", ms / 1000, ms % 1000));
        }
        if (limits.maxNodes) all.push_back(std::format("kata-set-param maxVisits {}", *limits.maxNodes));
        for (auto& c : commands) all.push_back(std::move(c));
        all.push_back(std::format("genmove {}", color(position.sideToMove())));

        firstId_ = nextId_;
        for (auto& c : all) lines.push_back(std::format("{} {}", nextId_++, c));
        lastId_ = nextId_ - 1;
        sink_ = &sink;
        position_ = position;
        inReply_ = false;
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

void GtpEngine::stop() {}  // See the class comment: a running genmove cannot be interrupted.

std::expected<void, EngineError> GtpEngine::sendRaw(std::string_view line) {
    if (!transport_.writeLine(line)) return std::unexpected(EngineError::TransportFailed);
    return {};
}

void GtpEngine::onReply(bool ok, std::optional<int> id, const std::string& text, EventSink*& deliver,
                        std::optional<SearchEvent>& event) {
    if (!sink_) return;  // late reply of a search that already ended
    if (id && (*id < firstId_ || *id > lastId_)) return;  // stale reply of an earlier search
    auto finish = [&](SearchEvent e) {
        deliver = sink_;
        event = std::move(e);
        sink_ = nullptr;
    };
    if (!ok) return finish(ErrorEvent{text});
    if (!id) return finish(ErrorEvent{"engine reply carries no id"});
    if (*id != lastId_) return;  // setup command accepted

    auto vertex = parse::parseGtpVertex(text, position_->width(), position_->height());
    if (!vertex) return finish(ErrorEvent{"engine returned an unreadable move: " + text});
    if (std::holds_alternative<parse::GtpPass>(*vertex)) return finish(PassEvent{});
    if (std::holds_alternative<parse::GtpResign>(*vertex)) return finish(ErrorEvent{"engine resigned"});
    const Coord c = std::get<Coord>(*vertex);
    if (position_->occupied(c)) return finish(ErrorEvent{"engine returned an illegal move: " + text});
    finish(BestMoveEvent{c});
}

void GtpEngine::onLine(std::string_view line) {
    EventSink* deliver = nullptr;
    std::optional<SearchEvent> event;
    {
        std::lock_guard lock(mutex_);
        if (!inReply_) {
            auto header = parse::parseGtpResponseHeader(line);
            if (!header) return;  // banner or text outside a reply
            inReply_ = true;
            replyOk_ = header->ok;
            replyId_ = header->id;
            replyText_ = std::move(header->body);
            return;
        }
        if (!line.empty()) {
            if (!replyText_.empty()) replyText_ += '\n';
            replyText_ += line;
            return;
        }
        inReply_ = false;
        onReply(replyOk_, replyId_, replyText_, deliver, event);
    }
    if (deliver) deliver->onEvent(*event);
}

void GtpEngine::onClosed(CloseReason) {
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
