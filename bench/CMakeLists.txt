include(FetchContent)
FetchContent_Declare(
        benchmark
        URL https://github.com/google/benchmark/archive/refs/tags/v1.9.4.tar.gz
        URL_HASH SHA256=b334658edd35efcf06a99d9be21e4e93e092bd5f95074c1673d5c8705d95c104
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

# Only meaningful in Release: Debug runs every invariant check.
add_executable(itch_bench
        feed_handler_bench.cc
        operations_bench.cc
)
target_include_directories(itch_bench PRIVATE ${PROJECT_SOURCE_DIR}/test)
target_link_libraries(itch_bench PRIVATE itch_core benchmark::benchmark_main)
target_compile_definitions(itch_bench PRIVATE ITCH_DATA_FILE="${ITCH_DATA_FILE}")
