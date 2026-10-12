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

## Usage

Place a Nasdaq BinaryFILE (ITCH 5.0) at `assets/20200130.BX_ITCH_50` (path is currently hardcoded in `main.cc`) and run:

```
./build/OrderBook
```

Sample data files are available from [Nasdaq's FTP server](https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/).

## Project Structure

```
├── main.cc                  # Framing loop, message dispatch, per-stock routing
├── include/
│   ├── parser.h             # Binary field extraction with endianness conversion
│   ├── reader.h             # File-to-buffer loader (binary mode)
│   ├── orderbook.h          # Order book interface (symbol, bids, asks)
│   ├── order.h              # Order struct (price, quantity, id, side, stock locate)
│   ├── handle.h             # O(1) lookup handle (price, side, list iterator, stock locate)
│   ├── types.h              # Type aliases (Price, Quantity, OrderId, StockLocate)
│   ├── side.h               # Buy/Sell enum
│   └── print.h              # Debug output for price levels
├── src/
│   └── orderbook.cc         # Order book operations (add, delete, execute, replace)
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
