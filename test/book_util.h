#ifndef ITCH_PROCESSOR_TEST_BOOK_UTIL_H_
#define ITCH_PROCESSOR_TEST_BOOK_UTIL_H_
#include <utility>
#include <vector>

#include "common/types.h"
#include "orderbook/order.h"

namespace itch_test {

// Aggregated (price, total quantity) per level, in book order.
template <typename T>
std::vector<std::pair<Price, Quantity>> Depth(const T& map) {
  std::vector<std::pair<Price, Quantity>> depth;
  for (const auto& [price, price_level] : map) {
    Quantity total = 0;
    for (const Order& order : price_level) {
      total += order.quantity;
    }
    depth.emplace_back(price, total);
  }
  return depth;
}

// Order ids in queue (FIFO) order.
inline std::vector<OrderId> QueueIds(const PriceLevel& price_level) {
  std::vector<OrderId> ids;
  for (const Order& order : price_level) {
    ids.push_back(order.order_id);
  }
  return ids;
}

}  // namespace itch_test

#endif  // ITCH_PROCESSOR_TEST_BOOK_UTIL_H_
