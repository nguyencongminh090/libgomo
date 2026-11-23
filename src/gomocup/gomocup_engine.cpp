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
        if (!sink_) return;
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
    {
        std::lock_guard lock(mutex_);
        sink = sink_;
        if (!sink) return;  // engine banner or reply outside a search
        if (std::holds_alternative<ErrorEvent>(event)) {
            // Engines such as Rapfi print non-fatal ERROR lines and still answer with a move.
            event = MessageEvent{std::string{line}};
        }
        if (auto* best = std::get_if<BestMoveEvent>(&event)) {
            if (!position_->contains(best->coord) || position_->occupied(best->coord)) {
                event = ErrorEvent{"engine returned an illegal move: " + std::string{line}};
            }
        }
        if (isTerminal(event)) sink_ = nullptr;
    }
    sink->onEvent(event);
}

void GomocupEngine::onClosed(CloseReason) {
    EventSink* sink = nullptr;
    {
        std::lock_guard lock(mutex_);
        closed_ = true;
        sink    = sink_;
        sink_   = nullptr;
    }
    if (sink) sink->onEvent(ErrorEvent{"engine connection closed"});
}

}  // namespace pygomo
