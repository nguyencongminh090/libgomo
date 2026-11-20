#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "pygomo/transport/transport.hpp"

namespace pygomo {

/// How to launch the child for ProcessTransport.
struct ProcessOptions {
    std::string program;                 ///< Looked up in PATH when it has no '/'.
    std::vector<std::string> args{};     ///< Arguments after the program name.
    std::string workingDirectory{};      ///< Empty: inherit.
    bool mergeStderr{false};             ///< false: the child's stderr goes to /dev/null.
    std::size_t maxLineLength{1U << 20}; ///< Longer lines end the transport with LineTooLong.
    /// After stdin is closed the child gets this long to exit, then SIGTERM, then this long
    /// again, then SIGKILL.
    std::chrono::milliseconds grace{400};
};

/// A Transport to a child process: the child's stdin/stdout are the channel.
///
/// Lines: a trailing '\r' is stripped, empty lines are delivered, a final line without a newline
/// is delivered at end of output. The listener runs on the reader thread and never overlaps
/// itself; it may call writeLine and close.
///
/// Closing: close() wakes the reader, ends the child (stdin closed, SIGTERM, SIGKILL) and reaps
/// it. When called from another thread it returns after that; when called from inside the
/// listener it returns at once and the cleanup finishes on the reader thread. onClosed is called
/// exactly once, after the child has been reaped, so exitStatus() is valid inside it.
///
/// The child runs in its own process group; when it ends, SIGKILL goes to the whole group (also if
/// the child exited by itself), and SIGTERM first if it had to be forced, so processes it started
/// die with it. A process that calls setsid leaves the group and survives. SIGPIPE is blocked only
/// on the thread inside writeLine, the process-wide disposition is not changed.
class ProcessTransport final : public Transport {
public:
    /// Stores the options; nothing is spawned until start().
    explicit ProcessTransport(ProcessOptions options);
    /// Closes the transport and joins the reader thread, so it waits for the child to be reaped.
    ~ProcessTransport() override;

    ProcessTransport(const ProcessTransport&) = delete;
    ProcessTransport& operator=(const ProcessTransport&) = delete;

    /// Spawns the child and starts the reader thread. Fails with AlreadyStarted, Closed (close()
    /// came first) or IoError (pipe or spawn failed; a missing program is reported here).
    [[nodiscard]] std::expected<void, TransportError> start(TransportListener& listener) override;
    /// Writes the line to the child's stdin; blocks while the pipe is full. Fails with
    /// InvalidLine, NotStarted, Closed or IoError (including a child that already exited).
    [[nodiscard]] std::expected<void, TransportError> writeLine(std::string_view line) override;
    /// Ends the child (see the class comment); blocks until it is reaped unless called from the
    /// listener. Idempotent.
    void close() override;

    /// Exit code of the child, or -N if it was killed by signal N. Empty until it has been reaped.
    [[nodiscard]] std::optional<int> exitStatus() const noexcept;

private:
    void readLoop();
    void terminateChild();
    CloseReason pump();

    ProcessOptions     options_;
    TransportListener* listener_{nullptr};
    std::thread        reader_;
    std::atomic<bool>  started_{false};
    std::atomic<bool>  requested_{false};

    std::mutex        writeMutex_;
    int               stdinFd_{-1};   // guarded by writeMutex_
    int               stdoutFd_{-1};  // reader thread only after start
    int               wakeRead_{-1};
    int               wakeWrite_{-1};
    int               pid_{-1};
    std::atomic<int>  exit_{0};
    std::atomic<bool> exited_{false};
};

}  // namespace pygomo
