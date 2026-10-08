# ITCH-Processor

A C++ ITCH 5.0 feed handler that reconstructs a limit order book from raw Nasdaq BinaryFILE data.

## Overview

Parses binary ITCH 5.0 messages from Nasdaq historical data files and maintains a live order book. The order book tracks price levels with sorted bids and asks, supports O(1) order lookup via handles, and processes Add, Delete, Execute, and Replace messages.

## Building

Requires CMake and a C++20-compatible compiler.

```
cmake -B build
cmake --build build
```

## Usage

Place a Nasdaq BinaryFILE (ITCH 5.0) in `assets/` and run:

```
./build/OrderBook
```

Sample data files are available from [Nasdaq's FTP server](https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/).

## Project Structure

```
├── main.cpp                 # Framing loop and message dispatch
├── include/
│   ├── Parser.h             # Binary field extraction with endianness conversion
│   ├── Reader.h             # File-to-buffer loader (binary mode)
│   ├── Orderbook.h          # Order book interface (bids, asks, order index)
│   ├── Order.h              # Order struct (price, quantity, id, side)
│   ├── Handle.h             # O(1) lookup handle (price, side, list iterator)
│   ├── Types.h              # Type aliases (Price, Quantity, OrderId)
│   ├── Side.h               # Buy/Sell enum
│   └── Print.h              # Debug output for price levels
├── src/
│   └── Orderbook.cpp        # Order book operations (add, delete, execute, replace)
└── CMakeLists.txt
```

## Supported ITCH Message Types

| Type | Message               | Action                                      |
|------|-----------------------|----------------------------------------------|
| A, F | Add Order             | Insert order at price level                  |
| D    | Order Delete          | Remove order from book                       |
| E    | Order Executed        | Reduce quantity; remove if fully filled       |
| U    | Order Replace         | Delete original, add replacement order        |

## Data Structures

- **Bids**: `std::map<Price, PriceLevelOrders, std::greater<>>` — sorted descending
- **Asks**: `std::map<Price, PriceLevelOrders, std::less<>>` — sorted ascending
- **Order index**: `std::unordered_map<OrderId, Handle>` — O(1) lookup by order ID
- **Price levels**: `std::list<Order>` — preserves FIFO queue priority

## Status

Work in progress. See [roadmap](#roadmap) for planned improvements.

## Roadmap

- [x] Naive limit order book with matching
- [x] Passive ITCH operations (Add, Delete, Execute, Replace)
- [x] ITCH 5.0 binary parser and framing loop
- [ ] Cancel and Executed-with-Price message types
- [ ] Full integration test with Nasdaq sample data
- [ ] Performance profiling and optimization
- [ ] Benchmarks
- [ ] CI pipeline
