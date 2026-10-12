cmake_minimum_required(VERSION 4.3)
project(ITCH-Processor)

set(CMAKE_CXX_STANDARD 20)

add_executable(OrderBook main.cc
        src/feed_handler/feed_handler.cc
        src/orderbook/orderbook.cc
)

target_include_directories(OrderBook PRIVATE src)
