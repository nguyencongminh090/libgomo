---
id: CORE-05-overall-architecture
type: design
size: M
status: active
owner: codex
created: 2026-11-22
supersedes: []
links: [CORE-03-document-architecture-lifecycle]
---
# PyGomo architecture

## Intent

PyGomo connects applications to Gomoku engines through a protocol-neutral `EnginePort`.
The application selects the concrete adapter and transport when it opens a session.

[Draw.io source](architecture.drawio) · [PNG with embedded Draw.io source](architecture.drawio.png)

## Structure

Arrows show direct CMake link dependencies. The parser links `pygomo::core` for the
protocol-neutral values and events it returns. [CMakeLists.txt](../../CMakeLists.txt)

```mermaid
flowchart LR
    adapters["gomocup / gtp / katagomo"] --> parse
    adapters --> core
    adapters --> transport
    parse --> core
    engine --> core
    engine --> threads["Threads::Threads"]
    process["process (POSIX)"] --> transport
    transport --> threads
```

## Interfaces

```mermaid
classDiagram
direction TB

class Application {
    <<client>>
}

class EnginePort {
    <<interface>>
    +supports(Rules) bool
    +setRules(Rules) expected
    +startSearch(Position, Limits, EventSink) expected
    +stop()
    +sendRaw(line) expected
}
class EventSink {
    <<interface>>
    +onEvent(SearchEvent)
}
class SearchEvent {
    <<variant>>
    InfoEvent / BestMoveEvent / SwapEvent / PassEvent
    ResignEvent / ErrorEvent / MessageEvent / UnparsedEvent
}
class Position
class Rules
class Limits
class Move
class Coord

class Transport {
    <<interface>>
    +start(TransportListener)
    +writeLine(line)
    +close()
}
class TransportListener {
    <<interface>>
    +onLine(line)
    +onClosed(CloseReason)
}
class StreamTransport
class ProcessTransport {
    +exitStatus() optional~int~
}
class ProcessOptions

class GomocupParser {
    <<functions>>
    +parseGomocupLine()
}
class GtpParser {
    <<functions>>
    +parseGtpResponseHeader()
    +parseGtpVertex()
    +formatGtpVertex()
    +parseGtpAnalyzeLine()
}

class GomocupEngine {
    +open() expected~void~
}
class GtpEngine {
    +open() expected~void~
}
class KataGomoEngine {
    +open() expected~void~
}
class DeadlineEngine {
    -inner_ EnginePort&
}
class BlockingSearch {
    <<function>>
    +search(EnginePort, Position, Limits) expected~SearchOutcome~
}
class SearchOutcome

Application ..> EnginePort
Application ..> BlockingSearch
BlockingSearch ..> EnginePort
BlockingSearch --> SearchOutcome

EnginePort ..> Position
EnginePort ..> Rules
EnginePort ..> Limits
EnginePort ..> EventSink
EventSink ..> SearchEvent
Position *-- Move
Move *-- Coord

EnginePort <|.. GomocupEngine
EnginePort <|.. GtpEngine
EnginePort <|.. KataGomoEngine
EnginePort <|.. DeadlineEngine
DeadlineEngine o--> EnginePort : decorates

TransportListener <|.. GomocupEngine
TransportListener <|.. GtpEngine
TransportListener <|.. KataGomoEngine
GomocupEngine --> Transport
GtpEngine --> Transport
KataGomoEngine --> Transport
Transport ..> TransportListener

GomocupEngine ..> GomocupParser
GtpEngine ..> GtpParser
KataGomoEngine ..> GtpParser

Transport <|.. StreamTransport
Transport <|.. ProcessTransport
ProcessTransport *-- ProcessOptions
```

## Session lifecycle

The example uses `GomocupEngine` and `ProcessTransport` on POSIX. The application owns the
transport, adapter, decorator and sink. The adapter borrows the transport, the transport
borrows the adapter as its listener, and the decorator borrows the adapter and the active
search sink. During a search, the adapter calls the relay owned by `DeadlineEngine`.

```mermaid
sequenceDiagram
    participant App as Application (control thread)
    participant Transport as ProcessTransport
    participant Adapter as GomocupEngine
    participant Deadline as DeadlineEngine
    participant Sink as EventSink

    App->>Transport: construct with ProcessOptions
    App->>Adapter: construct with transport
    App->>Deadline: construct with adapter
    App->>Adapter: open()
    Adapter->>Transport: start(adapter), spawn child and start reader

    Note over App,Deadline: Start the search after open() succeeds.
    App->>Deadline: startSearch(position, limits, sink)
    Deadline->>Adapter: startSearch(…, relay)
    Adapter->>Transport: writeLine() per command
    Transport->>Adapter: onLine(line) on the reader thread
    Adapter->>Deadline: onEvent(event) via relay
    Deadline->>Sink: onEvent(event)

    App->>Transport: close() from the control thread
    Transport->>Adapter: onClosed(reason) after the child is reaped
    Transport-->>App: returns after the reader joins
    Note over App,Deadline: Destroy DeadlineEngine (joins watchdog), then GomocupEngine, then ProcessTransport.
```

## Trade-offs

The application coordinates shutdown because it owns the session objects. It calls
`close()` from the control thread while the adapter, decorator and any active sink are
still alive. A close can deliver a final `ErrorEvent` for an active search. The sink can
be released once its terminal callback returns, including a timeout callback from the
watchdog. [ProcessTransport](../../include/pygomo/process/process_transport.hpp) ·
[GomocupEngine](../../include/pygomo/gomocup/gomocup_engine.hpp) ·
[DeadlineEngine](../../include/pygomo/engine/deadline_engine.hpp)
