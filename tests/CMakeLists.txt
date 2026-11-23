add_executable(pygomo_core_tests
  main.cpp
  core/coord_test.cpp
  core/position_test.cpp
  core/rules_test.cpp
  core/limits_test.cpp
  core/search_event_test.cpp
  core/engine_port_test.cpp
  core/command_port_test.cpp)
target_link_libraries(pygomo_core_tests PRIVATE pygomo::core)
# Vendored single header; SYSTEM keeps its own warnings out of our build.
target_include_directories(pygomo_core_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
pygomo_enable_warnings(pygomo_core_tests)
add_test(NAME port_sink COMMAND pygomo_core_tests "-tc=port_sink*")
add_test(NAME port_contract COMMAND pygomo_core_tests "-tc=port_contract*")
add_test(NAME cand1_defaults COMMAND pygomo_core_tests "-tc=cand1_defaults*")
add_test(NAME cand1_equality COMMAND pygomo_core_tests "-tc=cand1_equality*")
add_test(NAME cand1_compat COMMAND pygomo_core_tests "-tc=cand1_compat*")
add_test(NAME ext01_is_terminal COMMAND pygomo_core_tests "-tc=ext01_is_terminal*")
add_test(NAME ext02_core COMMAND pygomo_core_tests "-tc=ext02_*")
add_test(NAME ext05_core COMMAND pygomo_core_tests "-tc=ext05_*")
add_test(NAME ext06_core COMMAND pygomo_core_tests "-tc=ext06_*")
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
  engine/blocking_search_test.cpp
  engine/deadline_engine_test.cpp)
target_link_libraries(pygomo_engine_tests PRIVATE pygomo::engine)
target_include_directories(pygomo_engine_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
pygomo_enable_warnings(pygomo_engine_tests)
add_test(NAME port_blocking COMMAND pygomo_engine_tests "-tc=port_blocking*")
foreach(group timeout intime nodeadline callback serialization)
  add_test(NAME dead1_${group} COMMAND pygomo_engine_tests "-tc=dead1_${group}*")
endforeach()
add_test(NAME core4_failed_start COMMAND pygomo_engine_tests "-tc=core4_failed_start*")
add_test(NAME ext02_engine COMMAND pygomo_engine_tests "-tc=ext02_*")

add_executable(pygomo_gomocup_tests
  main.cpp
  gomocup/gomocup_engine_test.cpp
  gomocup/gomocup_encode_test.cpp
  gomocup/gomocup_command_test.cpp)
target_link_libraries(pygomo_gomocup_tests PRIVATE pygomo::gomocup pygomo::engine)
target_include_directories(pygomo_gomocup_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
pygomo_enable_warnings(pygomo_gomocup_tests)
foreach(group sequence rules events warning stop closed rect deadline)
  add_test(NAME gomo_${group} COMMAND pygomo_gomocup_tests "-tc=gomo_${group}*")
endforeach()
add_test(NAME ext02_gomocup COMMAND pygomo_gomocup_tests "-tc=ext02_*")
add_test(NAME ext03_gomocup COMMAND pygomo_gomocup_tests "-tc=ext03_*")
add_test(NAME ext05_gomocup COMMAND pygomo_gomocup_tests "-tc=ext05_*")
add_test(NAME ext09_gomocup COMMAND pygomo_gomocup_tests "-tc=ext09_gomocup*")
foreach(group default custom refuse board_block)
  add_test(NAME ext07_${group} COMMAND pygomo_gomocup_tests "-tc=ext07_${group}*")
endforeach()

if(UNIX)
  add_executable(pygomo_process_tests
    main.cpp
    process/process_transport_test.cpp)
  target_link_libraries(pygomo_process_tests PRIVATE pygomo::process)
  target_include_directories(pygomo_process_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
  pygomo_enable_warnings(pygomo_process_tests)
  foreach(group lines roundtrip close exit spawnfail limit group orphan sigpipe nohandler)
    add_test(NAME proc_${group} COMMAND pygomo_process_tests "-tc=proc_${group}*")
  endforeach()
endif()
add_test(NAME gomo2_parse COMMAND pygomo_parse_tests "-tc=gomo2_parse*")
add_test(NAME gomo2_fixtures COMMAND pygomo_parse_tests "-tc=gomo2_fixtures*")
foreach(group ranks forms fixture)
  add_test(NAME gomo7_${group} COMMAND pygomo_parse_tests "-tc=gomo7_${group}*")
endforeach()

# Needs a real Rapfi: run with PYGOMO_RAPFI=/path/to/pbrain-rapfi. Otherwise every test is skipped.
if(UNIX)
  add_executable(pygomo_live_tests
    main.cpp
    live/rapfi_live_test.cpp)
  target_link_libraries(pygomo_live_tests PRIVATE pygomo::gomocup pygomo::parse pygomo::process
                                                  pygomo::engine)
  target_include_directories(pygomo_live_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
  pygomo_enable_warnings(pygomo_live_tests)
  foreach(group live stop close deadline)
    add_test(NAME gomo2_${group} COMMAND pygomo_live_tests "-tc=gomo2_${group}*")
    set_tests_properties(gomo2_${group} PROPERTIES SKIP_REGULAR_EXPRESSION "SKIPPED")
  endforeach()
  add_test(NAME gomo7_live COMMAND pygomo_live_tests "-tc=gomo7_live*")
  set_tests_properties(gomo7_live PROPERTIES SKIP_REGULAR_EXPRESSION "SKIPPED")
  add_test(NAME ext09_live COMMAND pygomo_live_tests "-tc=ext09_live*")
  set_tests_properties(ext09_live PROPERTIES SKIP_REGULAR_EXPRESSION "SKIPPED")
endif()

foreach(group header vertex analyze total)
  add_test(NAME gtp_${group} COMMAND pygomo_parse_tests "-tc=gtp_${group}*")
endforeach()
foreach(group probe fixtures candidates robust)
  add_test(NAME gtp7_${group} COMMAND pygomo_parse_tests "-tc=gtp7_${group}*")
endforeach()

add_executable(pygomo_gtp_tests
  main.cpp
  gtp/gtp_engine_test.cpp
  gtp/gtp_command_test.cpp)
target_link_libraries(pygomo_gtp_tests PRIVATE pygomo::gtp)
target_include_directories(pygomo_gtp_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
pygomo_enable_warnings(pygomo_gtp_tests)
foreach(group sequence result rules lifecycle)
  add_test(NAME gtp2_${group} COMMAND pygomo_gtp_tests "-tc=gtp2_${group}*")
endforeach()
add_test(NAME ext05_gtp COMMAND pygomo_gtp_tests "-tc=ext05_*")
add_test(NAME ext09_gtp COMMAND pygomo_gtp_tests "-tc=ext09_gtp*")
foreach(group default builder custom analyze refuse)
  add_test(NAME ext08_gtp_${group} COMMAND pygomo_gtp_tests "-tc=ext08_gtp_${group}*")
endforeach()

add_executable(pygomo_katagomo_tests
  main.cpp
  katagomo/katagomo_engine_test.cpp
  katagomo/katagomo_command_test.cpp)
target_link_libraries(pygomo_katagomo_tests PRIVATE pygomo::katagomo)
target_include_directories(pygomo_katagomo_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
pygomo_enable_warnings(pygomo_katagomo_tests)
foreach(group sequence rules limits events)
  add_test(NAME kata_${group} COMMAND pygomo_katagomo_tests "-tc=kata_${group}*")
endforeach()
add_test(NAME ext05_katagomo COMMAND pygomo_katagomo_tests "-tc=ext05_*")
add_test(NAME ext09_kata COMMAND pygomo_katagomo_tests "-tc=ext09_kata*")
foreach(group default builder custom refuse)
  add_test(NAME ext08_kata_${group} COMMAND pygomo_katagomo_tests "-tc=ext08_kata_${group}*")
endforeach()
foreach(group sequence stream errors)
  add_test(NAME gtp3_${group} COMMAND pygomo_gtp_tests "-tc=gtp3_${group}*")
endforeach()
add_test(NAME gtp4_message COMMAND pygomo_gtp_tests "-tc=gtp4_message*")
add_test(NAME gtp5_text COMMAND pygomo_gtp_tests "-tc=gtp5_text*")
add_test(NAME gtp6_raw COMMAND pygomo_gtp_tests "-tc=gtp6_raw*")
add_test(NAME gtp7_engine COMMAND pygomo_gtp_tests "-tc=gtp7_engine*")
add_test(NAME port3_gtp COMMAND pygomo_gtp_tests "-tc=port3_gtp*")
add_test(NAME port3_kata COMMAND pygomo_katagomo_tests "-tc=port3_kata*")
add_test(NAME port3_gomocup COMMAND pygomo_gomocup_tests "-tc=port3_gomocup*")
add_test(NAME ext04_gtp COMMAND pygomo_gtp_tests "-tc=ext04_gtp*")
add_test(NAME ext04_kata COMMAND pygomo_katagomo_tests "-tc=ext04_kata*")
add_test(NAME ext02_kata COMMAND pygomo_katagomo_tests "-tc=ext02_kata*")
foreach(exe core gomocup gtp katagomo)
  add_test(NAME ext10_${exe} COMMAND pygomo_${exe}_tests "-tc=ext10_*")
endforeach()

add_executable(pygomo_deadline_adapter_tests
  main.cpp
  engine/deadline_adapters_test.cpp
  engine/deadline_command_test.cpp)
target_link_libraries(pygomo_deadline_adapter_tests PRIVATE pygomo::engine pygomo::gtp pygomo::katagomo)
target_include_directories(pygomo_deadline_adapter_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
pygomo_enable_warnings(pygomo_deadline_adapter_tests)
foreach(group gtp kata)
  add_test(NAME dead2_${group} COMMAND pygomo_deadline_adapter_tests "-tc=dead2_${group}*")
endforeach()
add_test(NAME ext10_deadline_adapters COMMAND pygomo_deadline_adapter_tests "-tc=ext10_*")

# Fails when a doctest case above is run by no ctest entry (needs the built tree).
add_test(NAME ctest_coverage
         COMMAND ${PROJECT_SOURCE_DIR}/scripts/check-ctest-coverage.sh ${PROJECT_BINARY_DIR})

# Style checks over the whole tree (rule: .claude/rules/coding-style.md).
add_test(NAME style_line_length
         COMMAND ${PROJECT_SOURCE_DIR}/scripts/check-line-length.sh include src tests)
add_test(NAME style_doc_comments
         COMMAND ${PROJECT_SOURCE_DIR}/scripts/check-doc-comments.sh include)
add_test(NAME style_format
         COMMAND ${PROJECT_SOURCE_DIR}/scripts/check-format.sh)
