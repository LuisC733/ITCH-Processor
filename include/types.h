#ifndef ITCH_PROCESSOR_TYPES_H_
#define ITCH_PROCESSOR_TYPES_H_
#include <cstdint>
#include <list>

struct Order;
using StockLocate = std::uint16_t;
using Price = std::uint32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;
using PriceLevel = std::list<Order>;

#endif  // ITCH_PROCESSOR_TYPES_H_
