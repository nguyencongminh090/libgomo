# libgomo

A C++23 layer between an application and Gomoku engines. The application talks to one interface,
`EnginePort`; adapters turn that into each engine's text protocol and parse the engine's output
back into typed events.

Goals: low coupling, several protocols behind one interface, open extension points (custom
parsers, encoders and commands), and clean parsing of engine output.

## Layers

```text
 application
     |  EnginePort / CommandPort
 DeadlineEngine, blocking search()      <- optional helpers (gomo::engine)
     |
 GomocupEngine | GtpEngine | KataGomoEngine   <- protocol adapters
     |            parsers (gomo::parse)
 Transport  (ProcessTransport, StreamTransport)
     |
 engine process
```

| Library | Role |
|---|---|
| `gomo::core` | Protocol-neutral types: `Position`, `Rules`, `Limits`, `SearchEvent`, `EnginePort`. No I/O, thread, process or protocol header (`scripts/check-core-deps.sh`). |
| `gomo::parse` | Pure text-to-event parsers for Gomocup and GTP lines. |
| `gomo::transport` | Line-oriented channel to an engine. |
| `gomo::process` | `ProcessTransport`: spawns the engine as a child process (POSIX). |
| `gomo::gomocup` | Adapter for Gomocup / Piskvork engines with the Yixin extensions (e.g. Rapfi). |
| `gomo::gtp` | Adapter for GTP engines with the KataGomo Gomoku extensions. |
| `gomo::katagomo` | Adapter for KataGomo's `gom` mode. Written from the source; not yet run against a real engine. |
| `gomo::engine` | `search()` (blocking) and `DeadlineEngine` (adds a time limit to any engine). |

Details: [`docs/design/architecture.md`](docs/design/architecture.md) and the developer guide
(`docs/guide/developer-guide.pdf`).

## Example

On POSIX, add the libgomo checkout to your CMake project. Save this as `CMakeLists.txt`
and replace `/path/to/libgomo` with the checkout path.

```cmake
cmake_minimum_required(VERSION 3.25)
project(first_search LANGUAGES CXX)
add_subdirectory("/path/to/libgomo" gomo)
add_executable(first_search main.cpp)
target_link_libraries(first_search PRIVATE gomo::gomocup gomo::process gomo::engine)
```

Save the program in `main.cpp`.

```cpp
#include <chrono>
#include <iostream>
#include <variant>

#include "gomo/engine/blocking_search.hpp"
#include "gomo/engine/deadline_engine.hpp"
#include "gomo/gomocup/gomocup_engine.hpp"
#include "gomo/process/process_transport.hpp"

int main(int argc, char** argv) {
    using namespace gomo;
    using namespace std::chrono_literals;

    if (argc != 2) {
        std::cerr << "usage: first_search /path/to/gomocup-engine\n";
        return 2;
    }

    auto position = Position::create(15, 15);
    if (!position || !position->play({7, 7}) || !position->play({8, 7})) return 1;

    ProcessTransport transport{ProcessOptions{.program = argv[1]}};
    GomocupEngine engine{transport};
    if (!engine.open()) return 1;
    DeadlineEngine bounded{engine};
    auto outcome = search(bounded, *position, Limits{.turnTime = 2000ms});
    transport.close();  // Joins the reader thread while the adapter and decorator are alive.
    if (!outcome) return 1;
    if (const auto* best = std::get_if<BestMoveEvent>(&outcome->terminal)) {
        std::cout << "best move: " << best->coord.x << ',' << best->coord.y << '\n';
        return 0;
    }
    if (const auto* error = std::get_if<ErrorEvent>(&outcome->terminal)) {
        std::cerr << "engine error: " << error->message << '\n';
    }
    return 1;
}
```

Build the example and pass the path to a Gomocup engine such as Rapfi. The program prints
the engine's move as a zero-based `x,y` coordinate.

```sh
cmake -S . -B build -G Ninja
cmake --build build
./build/first_search /path/to/pbrain-rapfi
```

For a non-blocking search, pass your own `EventSink` to `EnginePort::startSearch`.
Progress (`InfoEvent`) arrives on the adapter's reader thread. A terminal event reports
a best move, swap, pass, resignation, or error.

## Extending

- **Another output format:** pass a `parse::LineParser` to an adapter, or chain it with the default
  using `chainParsers(custom, parseGomocupLine)`. Unknown output can be returned as `CustomEvent`.
- **Another request shape:** pass a `RequestEncoder` to build the protocol lines of each search.
- **Engine-specific commands:** `CommandPort` (implemented by every adapter) sends a command and,
  if wanted, waits for a terminal event. See `docs/adr/ADR-02-extensible-engine-commands.md`.
- **Another protocol:** implement `EnginePort` (and optionally `CommandPort`) over any `Transport`.

## Build and test

Requires CMake >= 3.25, Ninja and a C++23 compiler (checked with g++ 15). Tests use doctest,
vendored in `third_party/`.

```sh
cmake --preset default && cmake --build --preset default   # Debug, warnings are errors
ctest --preset default                                     # all tests
ctest --preset default -R smoke                            # quick check
cmake --preset tsan && cmake --build --preset tsan && ctest --preset tsan   # ThreadSanitizer
```

Set `GOMO_RAPFI` to a Rapfi executable for the general live tests. Set `GOMO_RAPFI_V2`
and `GOMO_RAPFI_V3` to the corresponding executables for the version-specific force tests.
The rectangular-board test also uses `GOMO_RAPFI_V3`. Each live test skips when its
required executable setting is unset.

Checks: `scripts/check-core-deps.sh` (core dependency rule), `scripts/check-format.sh`
(clang-format 21.1.8), `node scripts/harness/harness-check.mjs` (docs lint).

## Migrating from PyGomo

Update the names in your application and CI configuration, then rebuild libgomo and the
application from source. Configure with a fresh build directory to replace cached settings
and archive paths.

| Surface | Before | After |
|---|---|---|
| CMake project | `PyGomo` | `libgomo` |
| Headers | `#include <pygomo/core/position.hpp>` | `#include <gomo/core/position.hpp>` |
| Namespaces | `pygomo`, `pygomo::parse`, `pygomo::encode` | `gomo`, `gomo::parse`, `gomo::encode` |
| CMake aliases | `pygomo::{core,transport,parse,engine,gomocup,gtp,katagomo,process}` | `gomo::` with the same components |
| CMake targets and archives | `pygomo_<component>`, `libpygomo_<component>.a` | `gomo_<component>`, `libgomo_<component>.a` |
| Build option and helper | `PYGOMO_WARNINGS_AS_ERRORS`, `pygomo_enable_warnings` | `GOMO_WARNINGS_AS_ERRORS`, `gomo_enable_warnings` |
| Test targets | `pygomo_*_tests`, `pygomo_smoke` | `gomo_*_tests`, `gomo_smoke` |
| Fixture definitions | `PYGOMO_FIXTURE_DIR`, `PYGOMO_GTP_FIXTURE_DIR` | `GOMO_FIXTURE_DIR`, `GOMO_GTP_FIXTURE_DIR` |
| Live test settings | `PYGOMO_RAPFI`, `PYGOMO_RAPFI_V2`, `PYGOMO_RAPFI_V3` | `GOMO_RAPFI`, `GOMO_RAPFI_V2`, `GOMO_RAPFI_V3` |

[Migration decision](docs/audit/AUD-09-libgomo-migration.md)

## Project status

Version 0.1.0. Known limits are listed in [`docs/handoff.md`](docs/handoff.md): GTP and KataGomo
engines cannot interrupt a search, and Rapfi V3 may drop a command written right after its move
(`docs/audit/AUD-08`).

Process and layout of `docs/`: see [`docs/README.md`](docs/README.md) and `CLAUDE.md`.
