#include "pygomo/katagomo/katagomo_engine.hpp"

#include <string>
#include <utility>
#include <vector>

#include "pygomo/katagomo/katagomo_encode.hpp"
#include "pygomo/parse/gomocup_parser.hpp"

namespace pygomo {

KataGomoEngine::KataGomoEngine(Transport& transport, parse::LineParser parser,
                               RequestEncoder encoder)
    : transport_(transport),
      parser_(parser ? std::move(parser) : parse::LineParser{parse::parseGomocupLine}),
      customEncoder_(static_cast<bool>(encoder)),
      encoder_(customEncoder_ ? std::move(encoder)
                              : RequestEncoder{encode::defaultKataGomoRequest}) {}

std::expected<void, EngineError> KataGomoEngine::open() {
    if (!transport_.start(*this)) return std::unexpected(EngineError::NotReady);
    return {};
}

bool KataGomoEngine::supports(const Rules& rules) const {
    return validate(rules).has_value() &&
           (customEncoder_ || (rules.opening == OpeningRule::FreeOpen && rules.extensions.empty()));
}

std::expected<void, EngineError> KataGomoEngine::setRules(const Rules& rules) {
    if (!supports(rules)) return std::unexpected(EngineError::Unsupported);
    std::lock_guard lock(mutex_);
    rules_ = rules;
    return {};
}

std::expected<void, EngineError> KataGomoEngine::startSearch(const Position& position,
                                                             const Limits&   limits,
                                                             EventSink&      sink) {
    std::vector<std::string> lines;
    std::uint64_t            id = 0;
    {
        std::lock_guard lock(mutex_);
        if (closed_) return std::unexpected(EngineError::NotReady);
        if (sink_) return std::unexpected(EngineError::Busy);
        // Nothing is sent for a request the encoder cannot express.
        auto encoded = encoder_(SearchRequest{rules_, position, limits});
        if (!encoded) return std::unexpected(encoded.error());
        if (encoded->empty()) return std::unexpected(EngineError::Unsupported);
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

void KataGomoEngine::stop() {}

std::expected<void, EngineError> KataGomoEngine::sendRaw(std::string_view line) {
    if (!transport_.writeLine(line)) return std::unexpected(EngineError::TransportFailed);
    return {};
}

void KataGomoEngine::onLine(std::string_view line) {
    if (line == "OK") return;  // reply to START, not an event
    SearchEvent event = parser_(line);
    EventSink*  sink  = nullptr;
    {
        std::lock_guard lock(mutex_);
        sink = sink_;
        if (!sink) return;  // banner or reply outside a search
        if (auto* best = std::get_if<BestMoveEvent>(&event)) {
            if (!position_->contains(best->coord) || position_->occupied(best->coord)) {
                event = ErrorEvent{"engine returned an illegal move: " + std::string{line}};
            }
        }
        if (isTerminal(event)) sink_ = nullptr;
    }
    sink->onEvent(event);
}

void KataGomoEngine::onClosed(CloseReason) {
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
