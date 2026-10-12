#include "orderbook.h"

#include <list>
#include <map>

#include "handle.h"
#include "order.h"
#include "side.h"
#include "types.h"

void Orderbook::AddOrder(const Order& order,
                         std::unordered_map<OrderId, Handle>& orders) {
  if (order.side == Side::kBuy) {
    HelperAddOrder(bids_, order, orders);
  } else {
    HelperAddOrder(asks_, order, orders);
  }
};
template <typename T>
void Orderbook::HelperAddOrder(T& map, Order order,
                               std::unordered_map<OrderId, Handle>& orders) {
  Handle handle;

  auto pair = map.insert(std::pair(order.price, PriceLevel{}));
  auto it = pair.first;
  auto location = it->second.insert(it->second.end(), order);

  handle.price = order.price;
  handle.side = order.side;
  handle.location = location;
  handle.stock_locate = order.stock_locate;
  orders.insert(std::pair(order.order_id, handle));
}
void Orderbook::DeleteOrder(
    const std::unordered_map<OrderId, Handle>::iterator it,
    std::unordered_map<OrderId, Handle>& orders) {
  if (it->second.side == Side::kBuy) {
    HelperDeleteOrder(bids_, it, orders);
  } else {
    HelperDeleteOrder(asks_, it, orders);
  }
};
template <typename T>
void Orderbook::HelperDeleteOrder(
    T& map, const std::unordered_map<OrderId, Handle>::iterator it,
    std::unordered_map<OrderId, Handle>& orders) {
  auto const list = map.find(it->second.price);
  PriceLevel& price_level = list->second;
  price_level.erase(it->second.location);
  if (price_level.empty()) {
    map.erase(list);
  }
  orders.erase(it);
};

void Orderbook::ReplaceOrder(
    const Order& order, const std::unordered_map<OrderId, Handle>::iterator it,
    std::unordered_map<OrderId, Handle>& orders) {
  DeleteOrder(it, orders);
  AddOrder(order, orders);
};
void Orderbook::OrderExecuted(
    const std::unordered_map<OrderId, Handle>::iterator it,
    const Quantity quantity, std::unordered_map<OrderId, Handle>& orders) {
  const auto order = it->second.location;
  order->quantity = order->quantity - quantity;
  if (order->quantity == 0) {
    DeleteOrder(it, orders);
  };
}