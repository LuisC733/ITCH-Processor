#ifndef ITCH_PROCESSOR_ORDERBOOK_H_
#define ITCH_PROCESSOR_ORDERBOOK_H_
#include <map>
#include <string>
#include <unordered_map>

#include "handle.h"
#include "order.h"
#include "types.h"

class Orderbook {
 public:
  std::string name_;

  Orderbook() = default;

  explicit Orderbook(const std::string& name) : name_(name) {}

  std::map<Price, PriceLevel, std::greater<>> bids_;
  std::map<Price, PriceLevel, std::less<>> asks_;

  void AddOrder(const Order& order,
                std::unordered_map<OrderId, Handle>& orders);
  template <typename T>
  void HelperAddOrder(T& map, Order order,
                      std::unordered_map<OrderId, Handle>& orders);
  void DeleteOrder(std::unordered_map<OrderId, Handle>::iterator it,
                   std::unordered_map<OrderId, Handle>& orders);
  template <typename T>
  void HelperDeleteOrder(T& map,
                         std::unordered_map<OrderId, Handle>::iterator it,
                         std::unordered_map<OrderId, Handle>& orders);
  void ReplaceOrder(const Order& order,
                    std::unordered_map<OrderId, Handle>::iterator it,
                    std::unordered_map<OrderId, Handle>& orders);
  void OrderExecuted(std::unordered_map<OrderId, Handle>::iterator it,
                     Quantity quantity,
                     std::unordered_map<OrderId, Handle>& orders);
};

#endif  // ITCH_PROCESSOR_ORDERBOOK_H_
