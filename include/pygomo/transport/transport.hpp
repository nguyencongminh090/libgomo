#pragma once

#include <expected>
#include <string_view>

namespace pygomo {

enum class CloseReason {
    Eof,          ///< The peer ended its output.
    Requested,    ///< close() was called.
    LineTooLong,  ///< A received line exceeded the configured limit.
    IoError,      ///< The underlying stream failed.
};

enum class TransportError {
    NotStarted,
    AlreadyStarted,
    Closed,
    InvalidLine,  ///< The line contains a newline and would break framing.
    IoError,
};

/// Receives what the peer sends. Implementations call it from their own reader thread,
/// never concurrently with themselves, and never after onClosed.
class TransportListener {
public:
    virtual ~TransportListener() = default;

    /// One received line, without its terminator. May be empty.
    virtual void onLine(std::string_view line) = 0;

    /// Called exactly once per started transport, when it ends for any reason.
    virtual void onClosed(CloseReason reason) = 0;
};

/// A line-oriented, full-duplex text channel to a peer (an engine). It knows nothing about
/// what the lines mean; a parser sits on top.
class Transport {
public:
    virtual ~Transport() = default;

    /// Begins delivering received lines to `listener`, which must outlive the transport.
    [[nodiscard]] virtual std::expected<void, TransportError> start(TransportListener& listener) = 0;

    /// Sends one line; the terminator is added by the transport. Safe to call from any thread.
    [[nodiscard]] virtual std::expected<void, TransportError> writeLine(std::string_view line) = 0;

    /// Stops the transport. Idempotent. After it returns, writeLine fails with Closed and the
    /// listener gets no further lines.
    virtual void close() = 0;
};

}  // namespace pygomo
