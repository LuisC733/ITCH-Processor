cmake_minimum_required(VERSION 4.3)
project(ITCH-Processor)

set(CMAKE_CXX_STANDARD 20)

# Invariant checks (ITCH_DCHECK) are active only without NDEBUG, i.e. in Debug.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(OrderBook main.cc
        src/feed_handler/feed_handler.cc
        src/orderbook/orderbook.cc
)

target_include_directories(OrderBook PRIVATE src)