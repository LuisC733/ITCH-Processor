#ifndef ORDERBOOK_TYPES_H
#define ORDERBOOK_TYPES_H
#include <cstdint>
#include <list>

class Order;
using Price = std::uint32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;
using PriceLevelOrders = std::list<Order>;

#endif //ORDERBOOK_TYPES_H
