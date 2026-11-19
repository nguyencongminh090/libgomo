cmake_minimum_required(VERSION 3.25)
project(PyGomo VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PYGOMO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

function(pygomo_enable_warnings target)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(PYGOMO_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  elseif(MSVC)
    target_compile_options(${target} PRIVATE /W4)
    if(PYGOMO_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  endif()
endfunction()

# Protocol-neutral core. It links nothing else (see scripts/check-core-deps.sh).
add_library(pygomo_core STATIC src/core/position.cpp)
add_library(pygomo::core ALIAS pygomo_core)
target_include_directories(pygomo_core PUBLIC include)
target_compile_features(pygomo_core PUBLIC cxx_std_23)
pygomo_enable_warnings(pygomo_core)

# Line-oriented text channel to an engine. Independent of the core; may use threads and streams.
find_package(Threads REQUIRED)
add_library(pygomo_transport STATIC src/transport/stream_transport.cpp)
add_library(pygomo::transport ALIAS pygomo_transport)
target_include_directories(pygomo_transport PUBLIC include)
target_compile_features(pygomo_transport PUBLIC cxx_std_23)
target_link_libraries(pygomo_transport PUBLIC Threads::Threads)
pygomo_enable_warnings(pygomo_transport)

# Pure text-to-event parsers. Depend on the core only.
add_library(pygomo_parse STATIC src/parse/gomocup_parser.cpp)
add_library(pygomo::parse ALIAS pygomo_parse)
target_include_directories(pygomo_parse PUBLIC include)
target_compile_features(pygomo_parse PUBLIC cxx_std_23)
target_link_libraries(pygomo_parse PUBLIC pygomo::core)
pygomo_enable_warnings(pygomo_parse)

# Blocking helpers on top of the engine port. Uses threads, so it is not part of the core.
add_library(pygomo_engine STATIC src/engine/blocking_search.cpp)
add_library(pygomo::engine ALIAS pygomo_engine)
target_include_directories(pygomo_engine PUBLIC include)
target_compile_features(pygomo_engine PUBLIC cxx_std_23)
target_link_libraries(pygomo_engine PUBLIC pygomo::core Threads::Threads)
pygomo_enable_warnings(pygomo_engine)

# Gomocup / Piskvork engine adapter.
add_library(pygomo_gomocup STATIC src/gomocup/gomocup_engine.cpp)
add_library(pygomo::gomocup ALIAS pygomo_gomocup)
target_include_directories(pygomo_gomocup PUBLIC include)
target_compile_features(pygomo_gomocup PUBLIC cxx_std_23)
target_link_libraries(pygomo_gomocup PUBLIC pygomo::core pygomo::parse pygomo::transport)
pygomo_enable_warnings(pygomo_gomocup)

enable_testing()
add_subdirectory(tests)
