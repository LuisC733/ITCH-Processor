#include "orderbook.h"

#include <list>
#include <map>

#include "handle.h"
#include "order.h"
#include "side.h"
#include "types.h"

PriceLevel::iterator Orderbook::AddOrder(const Order& order) {
  if (order.side == Side::kBuy) {
    return HelperAddOrder(bids_, order);
  }
  return HelperAddOrder(asks_, order);
}
template <typename T>
PriceLevel::iterator Orderbook::HelperAddOrder(T& map, const Order& order) {
  PriceLevel& price_level = map[order.price];
  return price_level.insert(price_level.end(), order);
}
void Orderbook::DeleteOrder(const Handle& handle) {
  if (handle.side == Side::kBuy) {
    HelperDeleteOrder(bids_, handle);
  } else {
    HelperDeleteOrder(asks_, handle);
  }
}
template <typename T>
void Orderbook::HelperDeleteOrder(T& map, const Handle& handle) {
  auto const list = map.find(handle.price);
  PriceLevel& price_level = list->second;
  price_level.erase(handle.location);
  if (price_level.empty()) {
    map.erase(list);
  }
}

bool Orderbook::ReduceQuantity(const Handle& handle, const Quantity quantity) {
  handle.location->quantity -= quantity;
  if (handle.location->quantity != 0) {
    return false;
  }
  DeleteOrder(handle);
  return true;
}
