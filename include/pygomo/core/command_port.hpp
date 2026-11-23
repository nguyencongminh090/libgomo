// Typed engine commands beside EnginePort (core layer, protocol neutral, pure data + interface).
// Planning extensible-engine-commands, section A.
#pragma once

#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "pygomo/core/engine_port.hpp"
#include "pygomo/core/search_event.hpp"
#include "pygomo/core/value.hpp"

namespace pygomo {

/// One command for an engine, as data. What goes on the wire is commandLines(command).
struct Command {
    /// Done once every line is written; the sink gets no event unless a queued command fails.
    struct None {};
    /// A search: ends with its terminal event (isTerminal), like startSearch, and is Busy until.
    struct Search {};
    /// Ends at the first event for which `done` returns true; that event still reaches the sink.
    /// The engine is Busy until then, or until the connection closes (ErrorEvent).
    struct Until {
        std::function<bool(const SearchEvent&)> done{};  ///< Called on the reader thread.
    };
    /// How the engine knows the command has finished.
    using Completion = std::variant<None, Search, Until>;

    std::string              name{};   ///< First word of the first line, when `lines` is empty.
    std::vector<std::string> lines{};  ///< Sent as is instead of `name` and `args` when not empty.
    std::vector<Value>       args{};   ///< Follow `name` on the first line, each via format(Value).
    std::vector<Value>       block{};  ///< One line per element via format(Value), after the head.
    std::optional<std::string> terminator{};        ///< Last line (e.g. `DONE`), when set.
    Completion                 completion{None{}};  ///< When the command is finished.
};

/// The lines that `command` puts on the wire, in order.
/// If `lines` is empty, the head is one line: `name` and the formatted `args`, joined by one space
/// (no head line when both are empty); otherwise the head is `lines` as is. Then one line per
/// `block` element (format(Value)), then `terminator` if set. An empty result is not sendable.
[[nodiscard]] std::vector<std::string> commandLines(const Command& command);

/// How execute treats an engine that is busy.
struct CommandOptions {
    /// False: Busy while a search or a Search/Until command runs. True: stop the running work
    /// (EnginePort::stop()), queue this command and write it only after the old terminal event
    /// has reached the old sink (normally on the reader thread). See each adapter for what it
    /// can stop.
    /// Rapfi V3 can still drop a command sent right after its move, and leaves the dropped
    /// command's arguments in its input (docs/audit/AUD-08 "Corrections to the planning").
    bool force{false};
};

/// Sends typed commands to an engine. An adapter that supports it implements this beside
/// EnginePort; EnginePort itself is unchanged.
class CommandPort {
  public:
    virtual ~CommandPort() = default;

    /// Sends `command`, or queues it when `options.force` is set and the engine is busy.
    /// Events of a Search or Until command go to `sink` from the reader thread; `sink` must
    /// outlive the command (until its terminal event, or until a queued command is written).
    /// Errors: Busy while the engine is busy (or, with force, while another command is queued),
    /// NotReady when the engine is gone, Unsupported when the command or its completion cannot be
    /// expressed (nothing is sent), TransportFailed when a line cannot be written.
    [[nodiscard]] virtual std::expected<void, EngineError> execute(const Command& command,
                                                                   EventSink&     sink,
                                                                   CommandOptions options = {}) = 0;
};

}  // namespace pygomo
