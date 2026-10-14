cmake_minimum_required(VERSION 3.24...4.3)
project(ITCH-Processor)

set(CMAKE_CXX_STANDARD 20)

# Invariant checks (ITCH_DCHECK) are active only without NDEBUG, i.e. in Debug.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(itch_core
        src/feed_handler/feed_handler.cc
        src/orderbook/orderbook.cc
)
target_include_directories(itch_core PUBLIC src)

add_executable(OrderBook main.cc)
target_link_libraries(OrderBook PRIVATE itch_core)

set(ITCH_DATA_FILE "${PROJECT_SOURCE_DIR}/assets/20200130.BX_ITCH_50"
        CACHE FILEPATH "ITCH 5.0 BinaryFILE for the integration test and benchmarks")

option(ITCH_BUILD_TESTS "Build the unit tests" ON)
if(ITCH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

option(ITCH_BUILD_BENCHMARKS "Build the benchmarks" ON)
if(ITCH_BUILD_BENCHMARKS)
    add_subdirectory(bench)
endif()
