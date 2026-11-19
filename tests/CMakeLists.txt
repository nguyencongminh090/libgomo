add_executable(pygomo_core_tests
  main.cpp
  core/coord_test.cpp
  core/position_test.cpp
  core/rules_test.cpp
  core/limits_test.cpp
  core/search_event_test.cpp)
target_link_libraries(pygomo_core_tests PRIVATE pygomo::core)
# Vendored single header; SYSTEM keeps its own warnings out of our build.
target_include_directories(pygomo_core_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
pygomo_enable_warnings(pygomo_core_tests)
add_test(NAME core_tests COMMAND pygomo_core_tests)

add_executable(pygomo_smoke smoke/smoke.cpp)
target_link_libraries(pygomo_smoke PRIVATE pygomo::core)
pygomo_enable_warnings(pygomo_smoke)
add_test(NAME smoke COMMAND pygomo_smoke)
