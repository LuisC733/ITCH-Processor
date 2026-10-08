cmake_minimum_required(VERSION 4.3)
project(ITCH-Processor)

set(CMAKE_CXX_STANDARD 20)

add_executable(OrderBook main.cpp
        src/Orderbook.cpp
        include/Reader.h
)

target_include_directories(OrderBook PUBLIC include)
