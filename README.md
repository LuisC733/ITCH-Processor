# ITCH-Processor

A C++ ITCH 5.0 feed handler that reconstructs per-stock limit order books from raw Nasdaq BinaryFILE data.

## Overview

Parses binary ITCH 5.0 messages from Nasdaq historical data files and maintains one live order book per stock. Books are created from Stock Directory messages and indexed by stock locate. Each book tracks price levels with sorted bids and asks; a shared order index provides O(1) lookup via handles and routes Delete, Execute, Cancel, and Replace messages to the right book.

## Building

Requires CMake and a C++20-compatible compiler.

```
cmake -B build
cmake --build build
```

Without an explicit build type this configures a Release build. For a Debug build, where order book invariants (book not crossed, no negative quantities, order index consistent with the books) are checked and abort on violation:

```
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

## Testing

Unit tests use GoogleTest, which CMake fetches on first configure. Pass `-DITCH_BUILD_TESTS=OFF` to skip them.

```
ctest --test-dir build-debug --output-on-failure
```

Death tests for the invariant checks only expect an abort in Debug builds.

## Usage

Place a Nasdaq BinaryFILE (ITCH 5.0) at `assets/20200130.BX_ITCH_50` (path is currently hardcoded in `main.cc`) and run:

```
./build/OrderBook
```

Sample data files are available from [Nasdaq's FTP server](https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/).

## Project Structure

```
├── main.cc                        # Entry point: loads the file and runs the feed handler
├── src/
│   ├── common/
│   │   ├── check.h                # Debug-only invariant checks (ITCH_DCHECK)
│   │   ├── types.h                # Type aliases (Price, Quantity, OrderId, StockLocate)
│   │   └── side.h                 # Buy/Sell enum
│   ├── feed_handler/
│   │   ├── feed_handler.h         # Feed handler interface (owns order books and order index)
│   │   └── feed_handler.cc        # Framing loop, message dispatch, per-stock routing
│   ├── orderbook/
│   │   ├── orderbook.h            # Order book interface (symbol, bids, asks)
│   │   ├── orderbook.cc           # Order book operations (add, delete, reduce quantity)
│   │   ├── order.h                # Order struct (price, quantity, id, side, stock locate)
│   │   ├── handle.h               # O(1) lookup handle (price, side, list iterator, stock locate)
│   │   └── print.h                # Debug output for price levels
│   ├── parser/
│   │   └── parser.h               # ITCH message decoding with endianness conversion
│   └── reader/
│       └── reader.h               # File-to-buffer loader (binary mode)
├── test/
│   ├── itch_builder.h             # Encodes ITCH messages and framed feeds for tests
│   ├── book_util.h                # Depth and queue views for assertions
│   ├── parser_test.cc             # Parser against hand-written spec bytes
│   ├── orderbook_test.cc          # Price/time priority, level cleanup, invariants
│   └── feed_handler_test.cc       # Message routing, order index, framing
└── CMakeLists.txt
```

## Supported ITCH Message Types

| Type | Message               | Action                                      |
|------|-----------------------|----------------------------------------------|
| R    | Stock Directory       | Create order book for stock locate           |
| A, F | Add Order             | Insert order at price level                  |
| D    | Order Delete          | Remove order from book                       |
| E    | Order Executed        | Reduce quantity; remove if fully filled      |
| C    | Executed with Price   | Reduce quantity; remove if fully filled      |
| X    | Order Cancel          | Reduce quantity; remove if fully cancelled   |
| U    | Order Replace         | Delete original, add replacement order       |

## Data Structures

- **Order books**: `std::vector<Orderbook>` with 65536 slots — indexed by stock locate
- **Bids**: `std::map<Price, PriceLevel, std::greater<>>` — sorted descending
- **Asks**: `std::map<Price, PriceLevel, std::less<>>` — sorted ascending
- **Order index**: `std::unordered_map<OrderId, Handle>` — shared across all books, O(1) lookup by order ID
- **Price levels**: `std::list<Order>` — preserves FIFO queue priority

## Status

Work in progress. See [roadmap](#roadmap) for planned improvements.

## Roadmap

- [x] Naive limit order book with matching
- [x] Passive ITCH operations (Add, Delete, Execute, Replace)
- [x] ITCH 5.0 binary parser and framing loop
- [x] Cancel and Executed-with-Price message types
- [x] Per-stock order books via Stock Directory messages
- [ ] Full integration test with Nasdaq sample data
- [ ] Performance profiling and optimization
- [ ] Benchmarks
- [ ] CI pipeline
