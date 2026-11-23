#include "pygomo/engine/deadline_engine.hpp"

#include <utility>
#include <variant>

namespace pygomo {

DeadlineEngine::DeadlineEngine(EnginePort& inner) : DeadlineEngine(inner, Options{}) {}

DeadlineEngine::DeadlineEngine(EnginePort& inner, Options options)
    : inner_(inner), options_(options) {
    commands_ = dynamic_cast<CommandPort*>(&inner_);
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

std::expected<void, EngineError> DeadlineEngine::setRules(const Rules& rules) {
    return inner_.setRules(rules);
}

std::expected<void, EngineError> DeadlineEngine::sendRaw(std::string_view line) {
    return inner_.sendRaw(line);
}

void DeadlineEngine::stop() { inner_.stop(); }

std::expected<void, EngineError> DeadlineEngine::startSearch(const Position& position,
                                                             const Limits&   limits,
                                                             EventSink&      sink) {
    std::uint64_t id = 0;
    {
        std::lock_guard lock(mutex_);
        if (detached_ || terminalPending_ || flushing_ || sink_) {
            return std::unexpected(EngineError::Busy);
        }
        // Installed before the call: the engine may deliver events before it returns.
        sink_     = &sink;
        id        = ++searchId_;
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
        std::optional<Queued> dropped;
        {
            std::lock_guard lock(mutex_);
            // The engine may already have ended the search through the relay (an adapter whose
            // transport closes mid-start does), and the sink may have started a newer search
            // from that callback or another thread. Clear only this call's search (CORE-04).
            if (searchId_ == id) {
                // A command forced over a search that never started has no terminal to wait for;
                // if the search did end through the relay, finishSearch owns the command.
                if (sink_) dropped = std::exchange(queued_, std::nullopt);
                sink_ = nullptr;
                deadline_.reset();
            }
        }
        if (dropped) dropped->sink->onEvent(ErrorEvent{"search failed to start; command not sent"});
    }
    return started;
}

void DeadlineEngine::Relay::onEvent(const SearchEvent& event) { owner_.onInnerEvent(event); }

void DeadlineEngine::onInnerEvent(const SearchEvent& event) {
    const bool       terminal = isTerminal(event);
    std::unique_lock lock(mutex_);
    if (detached_) {  // events of a search that timed out
        if (terminal) detached_ = false;
        return;
    }
    if (!sink_) return;
    if (terminal) {
        finishSearch(lock, event);
        return;
    }

    struct ProgressCallback {
        DeadlineEngine& engine;
        ~ProgressCallback() {
            std::lock_guard lock(engine.mutex_);
            if (--engine.progressCallbacks_ == 0 && engine.terminalPending_) {
                engine.cv_.notify_all();
            }
        }
    };
    auto* sink = sink_;
    ++progressCallbacks_;
    lock.unlock();
    const ProgressCallback callback{*this};
    sink->onEvent(event);
}

void DeadlineEngine::finishSearch(std::unique_lock<std::mutex>& lock, const SearchEvent& event,
                                  bool timedOut) {
    // Claim the terminal event now. Keep startSearch Busy until in-flight progress returns,
    // even if the timed-out engine answers while delivery is pending.
    auto* sink = std::exchange(sink_, nullptr);
    deadline_.reset();
    terminalPending_ = true;
    cv_.wait(lock, [this] { return progressCallbacks_ == 0; });
    terminalPending_ = false;
    // A kept command follows this terminal event. After a timeout the engine may still be
    // thinking and would drop it (AUD-03, AUD-08), so it is failed, never written; detached_
    // keeps this object Busy. Otherwise flushing_ keeps it Busy until the command is passed on.
    std::optional<Queued> dropped;
    const bool            kept = queued_.has_value() && !timedOut;
    if (timedOut) dropped = std::exchange(queued_, std::nullopt);
    if (kept) flushing_ = true;
    lock.unlock();
    sink->onEvent(event);
    if (dropped) dropped->sink->onEvent(ErrorEvent{"engine did not answer; command not sent"});
    if (!kept) return;
    // Passed on only now that the old sink has its terminal event, by whichever of this thread
    // and execute (still inside stop()) ends last.
    std::optional<Queued> ready;
    lock.lock();
    if (queued_) {
        queued_->delivered = true;
        if (!queued_->stopInFlight) ready = std::exchange(queued_, std::nullopt);
    } else {
        flushing_ = false;  // defensive: nothing left to pass on
    }
    lock.unlock();
    if (ready) passOn(std::move(*ready));
}

std::expected<void, EngineError> DeadlineEngine::execute(const Command& command, EventSink& sink,
                                                         CommandOptions options) {
    // Checked up front: a command kept during a search must not fail later for its shape.
    if (!canExecute(command)) return std::unexpected(EngineError::Unsupported);
    std::uint64_t queuedId = 0;
    {
        std::lock_guard lock(mutex_);
        // A timed-out engine may still be thinking: nothing is written until it answers (AUD-03).
        if (detached_ || terminalPending_ || flushing_) return std::unexpected(EngineError::Busy);
        if (sink_) {
            if (!options.force || queued_) return std::unexpected(EngineError::Busy);
            queuedId = ++queueId_;
            queued_  = Queued{command, &sink, queuedId, true, false};
        }
    }
    if (queuedId == 0) return commands_->execute(command, sink, options);

    inner_.stop();  // outside the lock: an engine may answer on this thread
    std::optional<Queued> released;
    {
        std::lock_guard lock(mutex_);
        if (queued_ && queued_->id == queuedId) {
            queued_->stopInFlight = false;
            if (queued_->delivered) released = std::exchange(queued_, std::nullopt);
        }
    }
    if (released) passOn(std::move(*released));
    return {};
}

bool DeadlineEngine::canExecute(const Command& command) const {
    return commands_ && commands_->canExecute(command);
}

// Hands a kept command to the wrapped engine after this object's search ended.
void DeadlineEngine::passOn(Queued queued) {
    auto sent = commands_->execute(queued.command, *queued.sink, {});
    {
        std::lock_guard lock(mutex_);
        flushing_ = false;
    }
    if (!sent) queued.sink->onEvent(ErrorEvent{"command could not be sent"});
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
        const bool changed =
            cv_.wait_until(lock, at, [&] { return quit_ || searchId_ != id || deadline_ != at; });
        if (changed) continue;

        if (!stopSent_) {
            stopSent_ = true;
            deadline_ = Clock::now() + options_.grace;
            lock.unlock();
            inner_.stop();
            lock.lock();
            continue;
        }
        // sink_ is set here: every path that clears sink_ also resets deadline_.
        detached_ = true;
        finishSearch(lock, ErrorEvent{"engine did not answer"}, true);
        lock.lock();
    }
}

}  // namespace pygomo
