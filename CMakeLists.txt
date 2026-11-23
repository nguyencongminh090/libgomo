cmake_minimum_required(VERSION 3.25)
project(libgomo VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GOMO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

function(gomo_enable_warnings target)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(GOMO_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  elseif(MSVC)
    target_compile_options(${target} PRIVATE /W4)
    if(GOMO_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  endif()
endfunction()

# Protocol-neutral core. It links nothing else (see scripts/check-core-deps.sh).
add_library(gomo_core STATIC src/core/position.cpp src/core/value.cpp src/core/command_port.cpp)
add_library(gomo::core ALIAS gomo_core)
target_include_directories(gomo_core PUBLIC include)
target_compile_features(gomo_core PUBLIC cxx_std_23)
gomo_enable_warnings(gomo_core)

# Line-oriented text channel to an engine. Independent of the core; may use threads and streams.
find_package(Threads REQUIRED)
add_library(gomo_transport STATIC src/transport/stream_transport.cpp)
add_library(gomo::transport ALIAS gomo_transport)
target_include_directories(gomo_transport PUBLIC include)
target_compile_features(gomo_transport PUBLIC cxx_std_23)
target_link_libraries(gomo_transport PUBLIC Threads::Threads)
gomo_enable_warnings(gomo_transport)

# Pure text-to-event parsers. Depend on the core only.
add_library(gomo_parse STATIC src/parse/gomocup_parser.cpp src/parse/gtp_parser.cpp)
add_library(gomo::parse ALIAS gomo_parse)
target_include_directories(gomo_parse PUBLIC include)
target_compile_features(gomo_parse PUBLIC cxx_std_23)
target_link_libraries(gomo_parse PUBLIC gomo::core)
gomo_enable_warnings(gomo_parse)

# Blocking helpers on top of the engine port. Uses threads, so it is not part of the core.
add_library(gomo_engine STATIC src/engine/blocking_search.cpp src/engine/deadline_engine.cpp)
add_library(gomo::engine ALIAS gomo_engine)
target_include_directories(gomo_engine PUBLIC include)
target_compile_features(gomo_engine PUBLIC cxx_std_23)
target_link_libraries(gomo_engine PUBLIC gomo::core Threads::Threads)
gomo_enable_warnings(gomo_engine)

# Gomocup / Piskvork engine adapter.
add_library(gomo_gomocup STATIC src/gomocup/gomocup_engine.cpp src/gomocup/gomocup_encode.cpp)
add_library(gomo::gomocup ALIAS gomo_gomocup)
target_include_directories(gomo_gomocup PUBLIC include)
target_compile_features(gomo_gomocup PUBLIC cxx_std_23)
target_link_libraries(gomo_gomocup PUBLIC gomo::core gomo::parse gomo::transport)
gomo_enable_warnings(gomo_gomocup)

# GTP engine adapter (KataGomo Gomoku extensions).
add_library(gomo_gtp STATIC src/gtp/gtp_engine.cpp src/gtp/gtp_encode.cpp)
add_library(gomo::gtp ALIAS gomo_gtp)
target_include_directories(gomo_gtp PUBLIC include)
target_compile_features(gomo_gtp PUBLIC cxx_std_23)
target_link_libraries(gomo_gtp PUBLIC gomo::core gomo::parse gomo::transport)
gomo_enable_warnings(gomo_gtp)

# KataGomo `gom` mode adapter.
add_library(gomo_katagomo STATIC src/katagomo/katagomo_engine.cpp src/katagomo/katagomo_encode.cpp)
add_library(gomo::katagomo ALIAS gomo_katagomo)
target_include_directories(gomo_katagomo PUBLIC include)
target_compile_features(gomo_katagomo PUBLIC cxx_std_23)
target_link_libraries(gomo_katagomo PUBLIC gomo::core gomo::parse gomo::transport)
gomo_enable_warnings(gomo_katagomo)

if(UNIX)
  add_library(gomo_process STATIC src/process/process_transport.cpp)
  add_library(gomo::process ALIAS gomo_process)
  target_include_directories(gomo_process PUBLIC include)
  target_compile_features(gomo_process PUBLIC cxx_std_23)
  target_link_libraries(gomo_process PUBLIC gomo::transport)
  gomo_enable_warnings(gomo_process)
endif()

enable_testing()
add_subdirectory(tests)
