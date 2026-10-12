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
