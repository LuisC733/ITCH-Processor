#ifndef ITCH_PROCESSOR_FEED_HANDLER_FEED_HANDLER_H_
#define ITCH_PROCESSOR_FEED_HANDLER_FEED_HANDLER_H_
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include "common/types.h"
#include "orderbook/handle.h"
#include "orderbook/order.h"
#include "orderbook/orderbook.h"

class FeedHandler {
 public:
  FeedHandler();

  void Process(const std::vector<uint8_t>& src);
  const Orderbook& book(StockLocate stock_locate) const;

 private:
  using OrderIndex = std::unordered_map<OrderId, Handle>;

  void ProcessMessage(const uint8_t* msg);
  void OnStockDirectory(const uint8_t* msg);
  void OnAddOrder(const uint8_t* msg);
  void OnDeleteOrder(const uint8_t* msg);
  void OnOrderExecuted(const uint8_t* msg);
  void OnReplaceOrder(const uint8_t* msg);
  void InsertOrder(const Order& order);
  void RemoveOrder(OrderIndex::iterator it);
  std::optional<OrderIndex::iterator> FindOrder(OrderId order_id);

  static constexpr int kMaxLocate = 65536;
  std::vector<Orderbook> books_;
  OrderIndex orders_;
};

#endif  // ITCH_PROCESSOR_FEED_HANDLER_FEED_HANDLER_H_
