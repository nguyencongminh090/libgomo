#include "pygomo/engine/blocking_search.hpp"

#include <condition_variable>
#include <mutex>
#include <type_traits>

namespace pygomo {
namespace {

class CollectingSink final : public EventSink {
  public:
    void onEvent(const SearchEvent& event) override {
        std::lock_guard lock(mutex_);
        if (done_) return;
        std::visit(
            [&](const auto& e) {
                using T = std::decay_t<decltype(e)>;
                if constexpr (std::is_same_v<T, InfoEvent>) {
                    outcome_.info.push_back(e);
                } else if constexpr (std::is_same_v<T, MessageEvent> ||
                                     std::is_same_v<T, UnparsedEvent>) {
                    outcome_.other.push_back(e);
                } else {
                    outcome_.terminal = e;
                    done_             = true;
                }
            },
            event);
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

}  // namespace pygomo
