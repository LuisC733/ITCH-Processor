#ifndef ITCH_PROCESSOR_ORDERBOOK_ORDERBOOK_H_
#define ITCH_PROCESSOR_ORDERBOOK_ORDERBOOK_H_
#include <map>
#include <string>

#include "common/types.h"
#include "orderbook/handle.h"
#include "orderbook/order.h"

class Orderbook {
 public:
  using Bids = std::map<Price, PriceLevel, std::greater<>>;
  using Asks = std::map<Price, PriceLevel, std::less<>>;

  Orderbook() = default;

  explicit Orderbook(const std::string& name) : name_(name) {}

  PriceLevel::iterator AddOrder(const Order& order);
  void DeleteOrder(const Handle& handle);
  bool ReduceQuantity(const Handle& handle, Quantity quantity);
  void CheckInvariants() const;

  const std::string& name() const { return name_; }
  const Bids& bids() const { return bids_; }
  const Asks& asks() const { return asks_; }

 private:
  template <typename T>
  PriceLevel::iterator HelperAddOrder(T& map, const Order& order);
  template <typename T>
  void HelperDeleteOrder(T& map, const Handle& handle);
  void CheckNotCrossed() const;

  std::string name_;
  Bids bids_;
  Asks asks_;
};

#endif  // ITCH_PROCESSOR_ORDERBOOK_ORDERBOOK_H_
