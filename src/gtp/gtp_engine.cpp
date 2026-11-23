#include "pygomo/gtp/gtp_engine.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <utility>
#include <variant>
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

// True if the engine replies to `line`: GTP 2 (section 2.4) skips empty and comment lines.
bool getsReply(std::string_view line) {
    const auto first = line.find_first_not_of(" \t\r");
    return first != std::string_view::npos && line[first] != '#';
}

}  // namespace

GtpEngine::GtpEngine(Transport& transport, parse::LineParser parser)
    : transport_(transport), parser_(std::move(parser)) {}

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

std::expected<void, EngineError> GtpEngine::startSearch(const Position& position,
                                                        const Limits& limits, EventSink& sink) {
    // Nothing is sent for a request the protocol cannot express.
    if (limits.matchTime || limits.maxDepth) return std::unexpected(EngineError::Unsupported);
    std::vector<std::string> commands;
    for (const Move& m : position.moves()) {
        auto vertex = parse::formatGtpVertex(m.coord, position.width(), position.height());
        if (!vertex) return std::unexpected(EngineError::Unsupported);
        commands.push_back(std::format("play {} {}", color(m.side), *vertex));
    }

    std::vector<std::string> lines;
    std::lock_guard          writeLock(writeMutex_);
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
        if (limits.maxNodes) {
            all.push_back(std::format("kata-set-param maxVisits {}", *limits.maxNodes));
        }
        for (auto& c : commands) all.push_back(std::move(c));
        analyzing_ = analyzeInterval_.has_value();
        if (analyzing_) {
            const auto cs = std::max<std::int64_t>(1, analyzeInterval_->count() / 10);
            all.push_back(std::format("kata-genmove_analyze {} interval {}",
                                      color(position.sideToMove()), cs));
        } else {
            all.push_back(std::format("genmove {}", color(position.sideToMove())));
        }

        firstId_ = nextId_;
        for (auto& c : all) lines.push_back(std::format("{} {}", nextId_++, c));
        pending_.insert(pending_.end(), lines.size(), false);
        lastId_   = nextId_ - 1;
        sink_     = &sink;
        position_ = position;
        // inReply_ is kept: a raw reply being read now must still be framed to its blank line.
    }
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (!transport_.writeLine(lines[i])) {
            std::lock_guard lock(mutex_);
            // The unwritten lines are the newest entries; no reply can have taken them.
            const auto unwritten = std::min(lines.size() - i, pending_.size());
            pending_.erase(pending_.end() - static_cast<std::ptrdiff_t>(unwritten), pending_.end());
            // A close or a rejected setup command already gave the sink its terminal event: that
            // is the one outcome, so report success (PORT-03). writeMutex_ keeps any newer search
            // out until this call returns, so a set sink_ is still this call's.
            if (!sink_) return {};
            sink_ = nullptr;
            return std::unexpected(EngineError::TransportFailed);
        }
    }
    return {};
}

void GtpEngine::setAnalyze(std::optional<std::chrono::milliseconds> interval) {
    std::lock_guard lock(mutex_);
    analyzeInterval_ = interval;
}

void GtpEngine::stop() {}  // See the class comment: a running genmove cannot be interrupted.

std::expected<void, EngineError> GtpEngine::sendRaw(std::string_view line) {
    const std::size_t replies = getsReply(line) ? 1 : 0;
    std::lock_guard   writeLock(writeMutex_);
    {
        // Queued before the write: the reply may be read before writeLine returns.
        std::lock_guard lock(mutex_);
        pending_.insert(pending_.end(), replies, true);
    }
    if (!transport_.writeLine(line)) {
        std::lock_guard lock(mutex_);
        const auto      unwritten = std::min(replies, pending_.size());
        pending_.erase(pending_.end() - static_cast<std::ptrdiff_t>(unwritten), pending_.end());
        return std::unexpected(EngineError::TransportFailed);
    }
    return {};
}

void GtpEngine::onReply(bool ok, std::optional<int> id, const std::string& text,
                        EventSink*& deliver, std::optional<SearchEvent>& event) {
    bool raw = false;
    if (!pending_.empty()) {
        raw = pending_.front();
        pending_.pop_front();
    }
    if (raw) {  // the answer to a raw command, with or without an id: never the search's end
        if (!sink_) return;
        std::string header = ok ? "=" : "?";
        if (id) header += std::to_string(*id);
        deliver = sink_;
        event   = MessageEvent{text.empty() ? header : header + " " + text};
        return;
    }
    if (!sink_) return;  // late reply of a search that already ended
    if (id && (*id < firstId_ || *id > lastId_)) return;  // stale reply of an earlier search
    auto finish = [&](SearchEvent e) {
        deliver = sink_;
        event   = std::move(e);
        sink_   = nullptr;
    };
    if (!ok) return finish(ErrorEvent{text});
    if (!id) return finish(ErrorEvent{"engine reply carries no id"});
    if (*id != lastId_) return;  // setup command accepted

    std::string_view move = text;
    if (analyzing_) {  // the reply body ends with `play <vertex>`
        if (!move.starts_with("play ")) {
            return finish(ErrorEvent{"engine reply has no final play: " + text});
        }
        move.remove_prefix(5);
    }
    auto vertex = parse::parseGtpVertex(move, position_->width(), position_->height());
    if (!vertex) return finish(ErrorEvent{"engine returned an unreadable move: " + text});
    if (std::holds_alternative<parse::GtpPass>(*vertex)) return finish(PassEvent{});
    if (std::holds_alternative<parse::GtpResign>(*vertex)) return finish(ResignEvent{});
    const Coord c = std::get<Coord>(*vertex);
    if (position_->occupied(c)) {
        return finish(ErrorEvent{"engine returned an illegal move: " + text});
    }
    finish(BestMoveEvent{c});
}

void GtpEngine::onLine(std::string_view line) {
    if (parser_ && !line.empty()) {
        int searchId = 0;
        {
            std::lock_guard lock(mutex_);
            // Check candidacy under the lock, then release it before invoking the user's parser
            // so the callback never runs while mutex_ is held.
            // A stale reply of an earlier search is not shown to this search's parser (AUD-04).
            const bool stale =
                inReply_ && replyId_ && (*replyId_ < firstId_ || *replyId_ > lastId_);
            if (sink_ && !stale && (inReply_ || !parse::parseGtpResponseHeader(line))) {
                searchId = lastId_;
            }
        }
        if (searchId != 0) {
            SearchEvent custom = parser_(line);
            if (!std::holds_alternative<UnparsedEvent>(custom) && !isTerminal(custom)) {
                EventSink* deliver = nullptr;
                {
                    std::lock_guard lock(mutex_);
                    if (sink_ && lastId_ == searchId) deliver = sink_;
                }
                if (deliver) deliver->onEvent(custom);
                return;
            }
        }
    }

    EventSink*                 deliver = nullptr;
    std::optional<SearchEvent> event;
    {
        std::lock_guard lock(mutex_);
        if (!inReply_) {
            auto header = parse::parseGtpResponseHeader(line);
            if (header) {
                inReply_   = true;
                replyOk_   = header->ok;
                replyId_   = header->id;
                replyText_ = std::move(header->body);
                return;
            }
            // Text outside a reply: free text while a search runs, otherwise a banner or late
            // output.
            if (!sink_ || line.empty()) return;
            deliver = sink_;
            event   = MessageEvent{std::string{line}};
        } else if (analyzing_ && replyId_ && *replyId_ == lastId_ && sink_ &&
                   line.starts_with("info ")) {
            deliver = sink_;  // streamed analysis line: report now, the reply is still open
            event   = parse::parseGtpAnalyzeLine(line, position_->width(), position_->height());
        } else if (!line.empty()) {
            if (!replyText_.empty()) replyText_ += '\n';
            replyText_ += line;
            return;
        } else {
            inReply_ = false;
            onReply(replyOk_, replyId_, replyText_, deliver, event);
        }
    }
    if (deliver) deliver->onEvent(*event);
}

void GtpEngine::onClosed(CloseReason) {
    EventSink* sink = nullptr;
    {
        std::lock_guard lock(mutex_);
        closed_ = true;
        pending_.clear();
        sink  = sink_;
        sink_ = nullptr;
    }
    if (sink) sink->onEvent(ErrorEvent{"engine connection closed"});
}

}  // namespace pygomo
