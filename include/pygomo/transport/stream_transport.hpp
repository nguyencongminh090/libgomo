#pragma once

#include <atomic>
#include <cstddef>
#include <iosfwd>
#include <mutex>
#include <thread>

#include "pygomo/transport/transport.hpp"

namespace pygomo {

struct StreamTransportOptions {
    /// Longest accepted line in raw characters before the newline. A longer line ends the
    /// transport with CloseReason::LineTooLong, so a misbehaving peer cannot exhaust memory.
    std::size_t maxLineLength{1U << 20};
};

/// A Transport over an already-connected std::istream / std::ostream pair (not owned).
///
/// A reader thread splits input into lines: a trailing '\r' is stripped, empty lines are
/// delivered, and a final line without a newline is delivered at end of input.
///
/// Closing: a read blocked inside the input stream cannot be interrupted. close() stops writing
/// and silences the listener at once, but the reader thread only ends when the stream reaches
/// end of input or fails. Whoever owns the underlying pipe or socket must close it, and the
/// destructor waits for that.
///
/// Threading: the listener runs on the reader thread, except that onClosed may also run on the
/// thread that calls close(). Calls into the listener are serialised, so it never overlaps
/// itself. The listener may call writeLine and close. start() must return before close() is
/// called from another thread.
class StreamTransport final : public Transport {
public:
    StreamTransport(std::istream& in, std::ostream& out, StreamTransportOptions options = {});
    ~StreamTransport() override;

    StreamTransport(const StreamTransport&) = delete;
    StreamTransport& operator=(const StreamTransport&) = delete;

    [[nodiscard]] std::expected<void, TransportError> start(TransportListener& listener) override;
    [[nodiscard]] std::expected<void, TransportError> writeLine(std::string_view line) override;
    void close() override;

private:
    void readLoop();
    void deliverLine(std::string_view line);
    void finish(CloseReason reason);

    std::istream& in_;
    std::ostream& out_;
    StreamTransportOptions options_;

    std::atomic<TransportListener*> listener_{nullptr};
    std::atomic<bool> closed_{false};
    bool closedNotified_{false};  // guarded by deliverMutex_

    std::mutex writeMutex_;
    std::recursive_mutex deliverMutex_;  // recursive: a listener may call close()
    std::thread reader_;
};

}  // namespace pygomo
