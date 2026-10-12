#include "orderbook/orderbook.h"

#include <list>
#include <map>

#include "common/check.h"
#include "common/side.h"
#include "common/types.h"
#include "orderbook/handle.h"
#include "orderbook/order.h"

namespace {

template <typename T>
void CheckSide(const std::string& name, const T& map, const Side side) {
  for (const auto& [price, price_level] : map) {
    ITCH_DCHECK(!price_level.empty())
        << "book=" << name << " empty price level at " << price;
    for (const Order& order : price_level) {
      ITCH_DCHECK(order.price == price)
          << "book=" << name << " order_id=" << order.order_id
          << " price=" << order.price << " level=" << price;
      ITCH_DCHECK(order.side == side)
          << "book=" << name << " order_id=" << order.order_id;
      ITCH_DCHECK(order.quantity > 0)
          << "book=" << name << " order_id=" << order.order_id;
    }
  }
}

}  // namespace

PriceLevel::iterator Orderbook::AddOrder(const Order& order) {
  ITCH_DCHECK(order.quantity > 0) << "order_id=" << order.order_id;
  const PriceLevel::iterator location = order.side == Side::kBuy
                                            ? HelperAddOrder(bids_, order)
                                            : HelperAddOrder(asks_, order);
  // Adding is the only operation that can cross the book.
  CheckNotCrossed();
  return location;
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
  ITCH_DCHECK(list != map.end())
      << "book=" << name_ << " order_id=" << handle.location->order_id
      << " price=" << handle.price;
  ITCH_DCHECK(handle.location->price == handle.price &&
              handle.location->side == handle.side)
      << "book=" << name_ << " order_id=" << handle.location->order_id;
  PriceLevel& price_level = list->second;
  price_level.erase(handle.location);
  if (price_level.empty()) {
    map.erase(list);
  }
}

bool Orderbook::ReduceQuantity(const Handle& handle, const Quantity quantity) {
  ITCH_DCHECK(quantity <= handle.location->quantity)
      << "book=" << name_ << " order_id=" << handle.location->order_id
      << " remaining=" << handle.location->quantity
      << " reduce_by=" << quantity;
  handle.location->quantity -= quantity;
  if (handle.location->quantity != 0) {
    return false;
  }
  DeleteOrder(handle);
  return true;
}

void Orderbook::CheckInvariants() const {
  CheckNotCrossed();
  CheckSide(name_, bids_, Side::kBuy);
  CheckSide(name_, asks_, Side::kSell);
}

void Orderbook::CheckNotCrossed() const {
  if (bids_.empty() || asks_.empty()) {
    return;
  }
  const Price best_bid = bids_.begin()->first;
  const Price best_ask = asks_.begin()->first;
  ITCH_DCHECK(best_bid < best_ask)
      << "book=" << name_ << " best_bid=" << best_bid
      << " best_ask=" << best_ask;
}
