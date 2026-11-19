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

enable_testing()
add_subdirectory(tests)
