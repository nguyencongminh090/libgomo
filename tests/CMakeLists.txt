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
  parse/gomocup_parser_test.cpp
  parse/rapfi_format_test.cpp
  parse/gtp_parser_test.cpp)
target_link_libraries(pygomo_parse_tests PRIVATE pygomo::parse)
target_compile_definitions(pygomo_parse_tests PRIVATE
  PYGOMO_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/gomocup"
  PYGOMO_GTP_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/gtp")
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

add_executable(pygomo_gomocup_tests
  main.cpp
  gomocup/gomocup_engine_test.cpp)
target_link_libraries(pygomo_gomocup_tests PRIVATE pygomo::gomocup pygomo::engine)
target_include_directories(pygomo_gomocup_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
pygomo_enable_warnings(pygomo_gomocup_tests)
foreach(group sequence rules events stop closed rect)
  add_test(NAME gomo_${group} COMMAND pygomo_gomocup_tests "-tc=gomo_${group}*")
endforeach()

if(UNIX)
  add_executable(pygomo_process_tests
    main.cpp
    process/process_transport_test.cpp)
  target_link_libraries(pygomo_process_tests PRIVATE pygomo::process)
  target_include_directories(pygomo_process_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
  pygomo_enable_warnings(pygomo_process_tests)
  foreach(group lines roundtrip close exit spawnfail limit group sigpipe nohandler)
    add_test(NAME proc_${group} COMMAND pygomo_process_tests "-tc=proc_${group}*")
  endforeach()
endif()
add_test(NAME gomo2_parse COMMAND pygomo_parse_tests "-tc=gomo2_parse*")
add_test(NAME gomo2_fixtures COMMAND pygomo_parse_tests "-tc=gomo2_fixtures*")

# Needs a real Rapfi: run with PYGOMO_RAPFI=/path/to/pbrain-rapfi. Otherwise every test is skipped.
if(UNIX)
  add_executable(pygomo_live_tests
    main.cpp
    live/rapfi_live_test.cpp)
  target_link_libraries(pygomo_live_tests PRIVATE pygomo::gomocup pygomo::process pygomo::engine)
  target_include_directories(pygomo_live_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
  pygomo_enable_warnings(pygomo_live_tests)
  foreach(group live stop close)
    add_test(NAME gomo2_${group} COMMAND pygomo_live_tests "-tc=gomo2_${group}*")
    set_tests_properties(gomo2_${group} PROPERTIES SKIP_REGULAR_EXPRESSION "SKIPPED")
  endforeach()
endif()

foreach(group header vertex analyze total)
  add_test(NAME gtp_${group} COMMAND pygomo_parse_tests "-tc=gtp_${group}*")
endforeach()

add_executable(pygomo_gtp_tests
  main.cpp
  gtp/gtp_engine_test.cpp)
target_link_libraries(pygomo_gtp_tests PRIVATE pygomo::gtp)
target_include_directories(pygomo_gtp_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
pygomo_enable_warnings(pygomo_gtp_tests)
foreach(group sequence result rules lifecycle)
  add_test(NAME gtp2_${group} COMMAND pygomo_gtp_tests "-tc=gtp2_${group}*")
endforeach()

add_executable(pygomo_katagomo_tests
  main.cpp
  katagomo/katagomo_engine_test.cpp)
target_link_libraries(pygomo_katagomo_tests PRIVATE pygomo::katagomo)
target_include_directories(pygomo_katagomo_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
pygomo_enable_warnings(pygomo_katagomo_tests)
foreach(group sequence rules limits events)
  add_test(NAME kata_${group} COMMAND pygomo_katagomo_tests "-tc=kata_${group}*")
endforeach()
foreach(group sequence stream errors)
  add_test(NAME gtp3_${group} COMMAND pygomo_gtp_tests "-tc=gtp3_${group}*")
endforeach()
add_test(NAME gtp4_message COMMAND pygomo_gtp_tests "-tc=gtp4_message*")
add_test(NAME gtp5_text COMMAND pygomo_gtp_tests "-tc=gtp5_text*")

# Fails when a doctest case above is run by no ctest entry (needs the built tree).
add_test(NAME ctest_coverage
         COMMAND ${PROJECT_SOURCE_DIR}/scripts/check-ctest-coverage.sh ${PROJECT_BINARY_DIR})
