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

### Integration test

`itch_integration_test` replays the full day from `ITCH_DATA_FILE` (CMake cache variable, defaults to `assets/20200130.BX_ITCH_50`) and compares the books at 9:30, 11:00, 13:00, 15:00, 16:00 and end of file against `test/integration/20200130.BX_ITCH_50.ref`: live orders, active books, a digest of every book's full depth, and the top 5 levels of the 10 most active symbols. The reference comes from `test/integration/reference.py`, an independent reconstruction in Python, so the test checks the implementation rather than recording its output. It is skipped when the data file is missing and takes about a minute in Debug (all invariant checks run) and 8 s in Release.

```
ctest --test-dir build-debug -L integration     # only the integration test
ctest --test-dir build-debug -LE integration    # everything else
python3 test/integration/reference.py <itch_file> <ref_file>   # regenerate (~1 min)
```

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
│   ├── integration/
│   │   ├── integration_test.cc    # Full-day replay against the reference
│   │   ├── reference.py           # Independent book reconstruction in Python
│   │   └── 20200130.BX_ITCH_50.ref  # Reference snapshots for the sample day
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
- [x] Full integration test with Nasdaq sample data
- [ ] Benchmarks of the naive version
- [ ] Performance profiling and optimization
- [ ] Benchmarks after optimization
- [ ] CI pipeline
