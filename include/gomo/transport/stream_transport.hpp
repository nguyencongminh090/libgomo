#pragma once

#include <atomic>
#include <cstddef>
#include <iosfwd>
#include <mutex>
#include <thread>

#include "gomo/transport/transport.hpp"

namespace gomo {

/// Settings for StreamTransport.
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
    /// Binds the two streams; they must outlive the transport. Reads nothing until start().
    StreamTransport(std::istream& in, std::ostream& out, StreamTransportOptions options = {});
    /// Closes the transport, then joins the reader thread; blocks until the input reaches end
    /// of input or fails (see the class comment).
    ~StreamTransport() override;

    StreamTransport(const StreamTransport&)            = delete;
    StreamTransport& operator=(const StreamTransport&) = delete;

    /// Starts the reader thread. Fails with AlreadyStarted, or Closed after close().
    [[nodiscard]] std::expected<void, TransportError> start(TransportListener& listener) override;
    /// Writes the line and flushes. Fails with NotStarted, InvalidLine, Closed or IoError.
    [[nodiscard]] std::expected<void, TransportError> writeLine(std::string_view line) override;
    /// Stops writing and silences the listener at once; does not wait for the reader thread.
    void close() override;

  private:
    void readLoop();
    void deliverLine(std::string_view line);
    void finish(CloseReason reason);

    std::istream&          in_;
    std::ostream&          out_;
    StreamTransportOptions options_;

    std::atomic<TransportListener*> listener_{nullptr};
    std::atomic<bool>               closed_{false};
    bool                            closedNotified_{false};  // guarded by deliverMutex_

    std::mutex           writeMutex_;
    std::recursive_mutex deliverMutex_;  // recursive: a listener may call close()
    std::thread          reader_;
};

}  // namespace gomo
