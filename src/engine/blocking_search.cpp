#include "gomo/engine/blocking_search.hpp"

#include <condition_variable>
#include <mutex>

namespace gomo {
namespace {

class CollectingSink final : public EventSink {
  public:
    void onEvent(const SearchEvent& event) override {
        std::lock_guard lock(mutex_);
        if (done_) return;
        if (isTerminal(event)) {
            outcome_.terminal = event;
            done_             = true;
        } else if (const auto* info = std::get_if<InfoEvent>(&event)) {
            outcome_.info.push_back(*info);
        } else {
            outcome_.other.push_back(event);
        }
        if (done_) cv_.notify_all();
    }

    SearchOutcome wait() {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [&] { return done_; });
        return std::move(outcome_);
    }

  private:
    std::mutex              mutex_;
    std::condition_variable cv_;
    SearchOutcome           outcome_{};
    bool                    done_ = false;
};

}  // namespace

std::expected<SearchOutcome, EngineError> search(EnginePort& engine, const Position& position,
                                                 const Limits& limits) {
    CollectingSink sink;
    if (auto started = engine.startSearch(position, limits, sink); !started) {
        return std::unexpected(started.error());
    }
    return sink.wait();
}

}  // namespace gomo
