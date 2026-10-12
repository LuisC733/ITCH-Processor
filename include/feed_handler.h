#ifndef ITCH_PROCESSOR_FEED_HANDLER_H_
#define ITCH_PROCESSOR_FEED_HANDLER_H_
#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include "handle.h"
#include "order.h"
#include "orderbook.h"
#include "types.h"

class FeedHandler {
 public:
  FeedHandler();

  void Process(const std::vector<uint8_t>& src);
  const Orderbook& book(StockLocate stock_locate) const;

 private:
  using OrderIndex = std::unordered_map<OrderId, Handle>;

  void ProcessMessage(const std::vector<uint8_t>& src, size_t offset);
  void OnStockDirectory(const std::vector<uint8_t>& src, size_t offset);
  void OnAddOrder(const std::vector<uint8_t>& src, size_t offset);
  void OnDeleteOrder(const std::vector<uint8_t>& src, size_t offset);
  void OnOrderExecuted(const std::vector<uint8_t>& src, size_t offset);
  void OnReplaceOrder(const std::vector<uint8_t>& src, size_t offset);
  void InsertOrder(const Order& order);
  void RemoveOrder(OrderIndex::iterator it);
  std::optional<OrderIndex::iterator> FindOrder(OrderId order_id);

  static constexpr int kMaxLocate = 65536;
  std::vector<Orderbook> books_;
  OrderIndex orders_;
};

#endif  // ITCH_PROCESSOR_FEED_HANDLER_H_
