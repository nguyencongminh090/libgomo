add_executable(pygomo_core_tests
  main.cpp
  core/coord_test.cpp
  core/position_test.cpp
  core/rules_test.cpp
  core/limits_test.cpp
  core/search_event_test.cpp
  core/engine_port_test.cpp)
target_link_libraries(pygomo_core_tests PRIVATE pygomo::core)
# Vendored single header; SYSTEM keeps its own warnings out of our build.
target_include_directories(pygomo_core_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
pygomo_enable_warnings(pygomo_core_tests)
add_test(NAME port_sink COMMAND pygomo_core_tests "-tc=port_sink*")
add_test(NAME port_contract COMMAND pygomo_core_tests "-tc=port_contract*")
add_test(NAME core_tests COMMAND pygomo_core_tests)

add_executable(pygomo_transport_tests
  main.cpp
  transport/stream_transport_test.cpp)
target_link_libraries(pygomo_transport_tests PRIVATE pygomo::transport)
target_include_directories(pygomo_transport_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
pygomo_enable_warnings(pygomo_transport_tests)
# One ctest entry per behaviour; each selects the doctest cases whose name starts with it.
foreach(group lines write close limit misc)
  add_test(NAME transport_${group}
           COMMAND pygomo_transport_tests "-tc=transport_${group}*")
endforeach()

add_executable(pygomo_smoke smoke/smoke.cpp)
target_link_libraries(pygomo_smoke PRIVATE pygomo::core)
pygomo_enable_warnings(pygomo_smoke)
add_test(NAME smoke COMMAND pygomo_smoke)

add_executable(pygomo_parse_tests
  main.cpp
  parse/gomocup_parser_test.cpp)
target_link_libraries(pygomo_parse_tests PRIVATE pygomo::parse)
target_include_directories(pygomo_parse_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
pygomo_enable_warnings(pygomo_parse_tests)
foreach(group messages move info unknown)
  add_test(NAME parse_${group}
           COMMAND pygomo_parse_tests "-tc=parse_${group}*")
endforeach()

add_executable(pygomo_engine_tests
  main.cpp
  engine/blocking_search_test.cpp)
target_link_libraries(pygomo_engine_tests PRIVATE pygomo::engine)
target_include_directories(pygomo_engine_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
pygomo_enable_warnings(pygomo_engine_tests)
add_test(NAME port_blocking COMMAND pygomo_engine_tests "-tc=port_blocking*")
