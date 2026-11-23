#include "pygomo/gomocup/gomocup_engine.hpp"

#include <string>

#include "pygomo/gomocup/gomocup_encode.hpp"
#include "pygomo/parse/gomocup_parser.hpp"

namespace pygomo {

GomocupEngine::GomocupEngine(Transport& transport, parse::LineParser parser, RequestEncoder encoder)
    : transport_(transport),
      parser_(parser ? std::move(parser) : parse::LineParser{parse::parseGomocupLine}),
      customEncoder_(static_cast<bool>(encoder)) {
    encoder_ = customEncoder_ ? std::move(encoder) : RequestEncoder{encode::defaultGomocupRequest};
}

std::expected<void, EngineError> GomocupEngine::open() {
    if (!transport_.start(*this)) return std::unexpected(EngineError::NotReady);
    return {};
}

bool GomocupEngine::supports(const Rules& rules) const {
    // A custom encoder decides what it can send, and refuses the rest at startSearch.
    if (customEncoder_) return validate(rules).has_value();
    if (!rules.extensions.empty()) return false;
    return encode::gomocupRuleId(rules).has_value() && validate(rules).has_value();
}

std::expected<void, EngineError> GomocupEngine::setRules(const Rules& rules) {
    if (!supports(rules)) return std::unexpected(EngineError::Unsupported);
    std::lock_guard lock(mutex_);
    rules_ = rules;
    return {};
}

std::expected<void, EngineError> GomocupEngine::startSearch(const Position& position,
                                                            const Limits& limits, EventSink& sink) {
    std::vector<std::string> lines;
    std::uint64_t            id = 0;
    {
        std::lock_guard lock(mutex_);
        if (closed_) return std::unexpected(EngineError::NotReady);
        if (sink_) return std::unexpected(EngineError::Busy);
        // Built before any state changes: an encoder error leaves no sink set and sends nothing.
        auto encoded = encoder_(SearchRequest{rules_, position, limits});
        if (!encoded) return std::unexpected(encoded.error());
        lines     = std::move(*encoded);
        sink_     = &sink;
        position_ = position;
        id        = ++searchId_;
    }
    for (const auto& l : lines) {
        if (!transport_.writeLine(l)) {
            std::lock_guard lock(mutex_);
            // The search already ended (a close or terminal line gave the sink its terminal
            // event): that is its one outcome, so report success, not a second one (PORT-03).
            if (!sink_ || searchId_ != id) return {};
            sink_ = nullptr;
            return std::unexpected(EngineError::TransportFailed);
        }
    }
    return {};
}

void GomocupEngine::stop() {
    {
        std::lock_guard lock(mutex_);
        if (!sink_ || sink_ == &discard_) return;
    }
    (void)transport_.writeLine("YXSTOP");
}

std::expected<void, EngineError> GomocupEngine::sendRaw(std::string_view line) {
    if (!transport_.writeLine(line)) return std::unexpected(EngineError::TransportFailed);
    return {};
}

void GomocupEngine::onLine(std::string_view line) {
    SearchEvent event = parser_(line);  // reader thread, outside the lock
    EventSink*  sink  = nullptr;
    std::function<bool(const SearchEvent&)> until;
    std::uint64_t                           id      = 0;
    bool                                    claimed = false;
    {
        std::lock_guard lock(mutex_);
        sink = sink_;
        // Engine banner or reply outside a search; a None command's reply is not awaited.
        if (!sink || sink == &discard_) return;
        if (std::holds_alternative<ErrorEvent>(event)) {
            // Engines such as Rapfi print non-fatal ERROR lines and still answer with a move.
            event = MessageEvent{std::string{line}};
        }
        if (auto* best = std::get_if<BestMoveEvent>(&event)) {
            // A Search command has no position to check against.
            if (position_ &&
                (!position_->contains(best->coord) || position_->occupied(best->coord))) {
                event = ErrorEvent{"engine returned an illegal move: " + std::string{line}};
            }
        }
        if (until_) {
            until = until_;  // a copy: the user's predicate runs outside the lock
            id    = searchId_;
        } else if (isTerminal(event)) {
            claimed = finishLocked();
        }
    }
    if (until && until(event)) {
        std::lock_guard lock(mutex_);
        if (sink_ == sink && searchId_ == id) claimed = finishLocked();
    }
    sink->onEvent(event);
    if (!claimed) return;
    // Written only after the terminal event reached the old sink: Rapfi drops commands while
    // thinking. Whichever of this thread and execute (still writing YXSTOP) ends last writes it.
    bool flush = false;
    {
        std::lock_guard lock(mutex_);
        if (queued_ && queued_->active) {
            queued_->delivered = true;
            flush              = !queued_->stopInFlight;
        }
    }
    if (flush) flushQueued();
}

void GomocupEngine::onClosed(CloseReason) {
    EventSink* sink    = nullptr;
    EventSink* dropped = nullptr;  // a queued command that was never claimed
    {
        std::lock_guard lock(mutex_);
        closed_ = true;
        sink    = sink_;
        sink_   = nullptr;
        until_  = nullptr;
        // A claimed None command reports on its own sink; a plain None write reports by return.
        if (sink == &discard_) sink = queued_ && queued_->active ? queued_->sink : nullptr;
        if (queued_ && !queued_->active) dropped = queued_->sink;
        queued_.reset();
    }
    if (sink) sink->onEvent(ErrorEvent{"engine connection closed"});
    if (dropped) dropped->onEvent(ErrorEvent{"engine connection closed; command not sent"});
}

std::expected<void, EngineError> GomocupEngine::execute(const Command& command, EventSink& sink,
                                                        CommandOptions options) {
    auto lines = commandLines(command);
    if (lines.empty()) return std::unexpected(EngineError::Unsupported);
    const auto* until = std::get_if<Command::Until>(&command.completion);
    if (until && !until->done) return std::unexpected(EngineError::Unsupported);
    const bool    none     = std::holds_alternative<Command::None>(command.completion);
    std::uint64_t id       = 0;
    std::uint64_t queuedId = 0;
    {
        std::lock_guard lock(mutex_);
        if (closed_) return std::unexpected(EngineError::NotReady);
        if (sink_) {
            // A None command being written cannot be stopped and has no terminal event to wait for.
            if (!options.force || queued_ || sink_ == &discard_) {
                return std::unexpected(EngineError::Busy);
            }
            queuedId = ++queueId_;
            queued_  = Queued{
                std::move(lines), command.completion, &sink, queuedId, true, false, 0, false};
        } else {
            id = beginCommand(command.completion, sink);
        }
    }
    if (id != 0) return writeCommand(lines, id, none);

    // Written outside the lock: a transport may answer on this thread. If the move arrives
    // before YXSTOP is written, the side that finishes last (this thread or the reader, once the
    // old sink has its terminal event) writes the command, so YXSTOP never follows it.
    (void)transport_.writeLine("YXSTOP");
    bool flush = false;
    {
        std::lock_guard lock(mutex_);
        if (queued_ && queued_->id == queuedId) {
            queued_->stopInFlight = false;
            flush                 = queued_->active && queued_->delivered;
        }
    }
    if (flush) flushQueued();
    return {};
}

// Caller holds the lock. Marks the engine Busy for a command and returns its search id.
std::uint64_t GomocupEngine::beginCommand(const Command::Completion& completion, EventSink& sink) {
    if (std::holds_alternative<Command::None>(completion)) {
        sink_ = &discard_;
    } else {
        sink_ = &sink;
        position_.reset();  // a command has no position: no illegal-move check
        const auto* until = std::get_if<Command::Until>(&completion);
        until_            = until ? until->done : nullptr;
    }
    return ++searchId_;
}

// Caller holds the lock. Ends the running search or command; a queued command takes its place
// at once, so nothing can start between this terminal event and the command's write. Returns
// true if it claimed a queued command, to be written once the terminal event is delivered.
bool GomocupEngine::finishLocked() {
    sink_  = nullptr;
    until_ = nullptr;
    if (!queued_ || queued_->active) return false;
    queued_->active   = true;
    queued_->activeId = beginCommand(queued_->completion, *queued_->sink);
    return true;
}

// Writes the claimed queued command; a failure is reported on its sink, the one outcome left.
void GomocupEngine::flushQueued() {
    std::vector<std::string> lines;
    EventSink*               user = nullptr;
    std::uint64_t            id   = 0;
    bool                     none = false;
    {
        std::lock_guard lock(mutex_);
        if (!queued_ || !queued_->active) return;
        lines = std::move(queued_->lines);
        user  = queued_->sink;
        id    = queued_->activeId;
        none  = std::holds_alternative<Command::None>(queued_->completion);
    }
    bool failed = false;
    for (const auto& l : lines) {
        if (!transport_.writeLine(l)) {
            failed = true;
            break;
        }
    }
    EventSink* report = nullptr;
    {
        std::lock_guard lock(mutex_);
        if (queued_ && queued_->activeId == id) queued_.reset();
        // Not ours any more: a close or the command's own terminal event already ended it.
        const bool ours = searchId_ == id && sink_ == (none ? &discard_ : user);
        if (ours && (failed || none)) {
            sink_  = nullptr;
            until_ = nullptr;
        }
        if (ours && failed) report = user;
    }
    if (report) report->onEvent(ErrorEvent{"command could not be sent"});
}

// Writes a command started on the caller's thread; errors are returned, not delivered.
std::expected<void, EngineError> GomocupEngine::writeCommand(const std::vector<std::string>& lines,
                                                             std::uint64_t id, bool none) {
    for (const auto& l : lines) {
        if (!transport_.writeLine(l)) {
            std::lock_guard lock(mutex_);
            if (none) {
                if (sink_ == &discard_ && searchId_ == id) sink_ = nullptr;
                return std::unexpected(EngineError::TransportFailed);
            }
            // As in startSearch: an ended command already had its one outcome (PORT-03).
            if (!sink_ || searchId_ != id) return {};
            sink_  = nullptr;
            until_ = nullptr;
            return std::unexpected(EngineError::TransportFailed);
        }
    }
    if (none) {
        std::lock_guard lock(mutex_);
        if (sink_ == &discard_ && searchId_ == id) sink_ = nullptr;
    }
    return {};
}

}  // namespace pygomo
