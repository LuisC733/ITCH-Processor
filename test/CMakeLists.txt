include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
        URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(itch_tests
        feed_handler_test.cc
        orderbook_test.cc
        parser_test.cc
)
target_link_libraries(itch_tests PRIVATE itch_core GTest::gmock_main)

include(GoogleTest)
gtest_discover_tests(itch_tests)

# Full-day replay against reference.py; skipped when ITCH_DATA_FILE is missing.
# Excluded from a quick run with: ctest -LE integration
add_executable(itch_integration_test integration/integration_test.cc)
target_link_libraries(itch_integration_test PRIVATE itch_core GTest::gmock_main)
target_compile_definitions(itch_integration_test PRIVATE
        ITCH_DATA_FILE="${ITCH_DATA_FILE}"
        ITCH_REFERENCE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/integration/20200130.BX_ITCH_50.ref"
)
gtest_discover_tests(itch_integration_test PROPERTIES LABELS integration)
