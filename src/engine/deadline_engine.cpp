#include "pygomo/engine/deadline_engine.hpp"

#include <variant>

namespace pygomo {

DeadlineEngine::DeadlineEngine(EnginePort& inner) : DeadlineEngine(inner, Options{}) {}

DeadlineEngine::DeadlineEngine(EnginePort& inner, Options options)
    : inner_(inner), options_(options) {
    watchdog_ = std::thread([this] { watchdog(); });
}

DeadlineEngine::~DeadlineEngine() {
    {
        std::lock_guard lock(mutex_);
        quit_ = true;
    }
    cv_.notify_all();
    watchdog_.join();
}

bool DeadlineEngine::supports(const Rules& rules) const { return inner_.supports(rules); }

std::expected<void, EngineError> DeadlineEngine::setRules(const Rules& rules) { return inner_.setRules(rules); }

std::expected<void, EngineError> DeadlineEngine::sendRaw(std::string_view line) { return inner_.sendRaw(line); }

void DeadlineEngine::stop() { inner_.stop(); }

std::expected<void, EngineError> DeadlineEngine::startSearch(
    const Position& position, const Limits& limits, EventSink& sink) {
    {
        std::lock_guard lock(mutex_);
        if (detached_ || sink_) return std::unexpected(EngineError::Busy);
        // Installed before the call: the engine may deliver events before it returns.
        sink_ = &sink;
        ++searchId_;
        stopSent_ = false;
        if (limits.turnTime && limits.turnTime->count() > 0) {
            deadline_ = Clock::now() + *limits.turnTime + options_.grace;
        } else {
            deadline_.reset();
        }
    }
    cv_.notify_all();
    auto started = inner_.startSearch(position, limits, relay_);
    if (!started) {
        std::lock_guard lock(mutex_);
        // The engine may already have ended the search through the relay; then sink_ is null.
        sink_ = nullptr;
        deadline_.reset();
    }
    return started;
}

void DeadlineEngine::Relay::onEvent(const SearchEvent& event) { owner_.onInnerEvent(event); }

void DeadlineEngine::onInnerEvent(const SearchEvent& event) {
    const bool terminal = !std::holds_alternative<InfoEvent>(event) &&
                          !std::holds_alternative<MessageEvent>(event) &&
                          !std::holds_alternative<UnparsedEvent>(event);
    EventSink* sink = nullptr;
    {
        std::lock_guard lock(mutex_);
        if (detached_) {  // events of a search that timed out
            if (terminal) detached_ = false;
            return;
        }
        sink = sink_;
        if (!sink) return;
        if (terminal) {
            sink_ = nullptr;
            deadline_.reset();
        }
    }
    sink->onEvent(event);
}

// One long-lived thread: it never joins itself, so a sink may start the next search from onEvent.
void DeadlineEngine::watchdog() {
    std::unique_lock lock(mutex_);
    while (!quit_) {
        if (!deadline_) {
            cv_.wait(lock, [this] { return quit_ || deadline_.has_value(); });
            continue;
        }
        const auto at = *deadline_;
        const auto id = searchId_;
        if (cv_.wait_until(lock, at, [&] { return quit_ || searchId_ != id || deadline_ != at; })) continue;

        if (!stopSent_) {
            stopSent_ = true;
            deadline_ = Clock::now() + options_.grace;
            lock.unlock();
            inner_.stop();
            lock.lock();
            continue;
        }
        EventSink* sink = sink_;
        sink_ = nullptr;
        deadline_.reset();
        detached_ = sink != nullptr;
        lock.unlock();
        if (sink) sink->onEvent(ErrorEvent{"engine did not answer"});
        lock.lock();
    }
}

}  // namespace pygomo
