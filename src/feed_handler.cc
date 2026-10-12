#include "feed_handler.h"

#include <cstring>
#include <iostream>

#include "order.h"
#include "parser.h"

FeedHandler::FeedHandler() : books_(kMaxLocate) {}

void FeedHandler::Process(const std::vector<uint8_t>& src) {
  size_t offset = 0;
  while (offset + 2 <= src.size()) {
    uint16_t dest = {};
    memcpy(&dest, src.data() + offset, sizeof(dest));
    const uint16_t length = __builtin_bswap16(dest);
    offset += 2;
    if (length == 0 || offset + length > src.size()) {
      std::cerr << "Error: Truncated message at offset: " << offset
                << std::endl;
      break;
    }
    ProcessMessage(src, offset);
    offset += length;
  }
}

const Orderbook& FeedHandler::book(const StockLocate stock_locate) const {
  return books_[stock_locate];
}

void FeedHandler::ProcessMessage(const std::vector<uint8_t>& src,
                                 const size_t offset) {
  switch (src[offset]) {
    case 'R':
      OnStockDirectory(src, offset);
      break;
    case 'A':
    case 'F':
      OnAddOrder(src, offset);
      break;
    case 'D':
      OnDeleteOrder(src, offset);
      break;
    case 'E':
    case 'C':
    case 'X':
      OnOrderExecuted(src, offset);
      break;
    case 'U':
      OnReplaceOrder(src, offset);
      break;
    default:
      break;
  }
}

void FeedHandler::OnStockDirectory(const std::vector<uint8_t>& src,
                                   const size_t offset) {
  const auto [stock_locate, name] = ParseStockDirectory(src, offset);
  books_[stock_locate] = Orderbook{name};
}

void FeedHandler::OnAddOrder(const std::vector<uint8_t>& src,
                             const size_t offset) {
  InsertOrder(ParseAddOrder(src, offset));
}

void FeedHandler::OnDeleteOrder(const std::vector<uint8_t>& src,
                                const size_t offset) {
  const auto found = FindOrder(ParseOrderId(src, offset));
  if (!found) {
    return;
  }
  RemoveOrder(*found);
}

void FeedHandler::OnOrderExecuted(const std::vector<uint8_t>& src,
                                  const size_t offset) {
  const auto found = FindOrder(ParseOrderId(src, offset));
  if (!found) {
    return;
  }
  const auto it = *found;
  if (books_[it->second.stock_locate].ReduceQuantity(
          it->second, ParseOrderQuantity(src, offset))) {
    orders_.erase(it);
  }
}

void FeedHandler::OnReplaceOrder(const std::vector<uint8_t>& src,
                                 const size_t offset) {
  const auto found = FindOrder(ParseOrderId(src, offset));
  if (!found) {
    return;
  }
  Order order = ParseReplaceOrder(src, offset);
  order.side = (*found)->second.side;
  RemoveOrder(*found);
  InsertOrder(order);
}

void FeedHandler::InsertOrder(const Order& order) {
  const PriceLevel::iterator location =
      books_[order.stock_locate].AddOrder(order);
  orders_.emplace(order.order_id, Handle{order.price, order.side, location,
                                         order.stock_locate});
}

void FeedHandler::RemoveOrder(const OrderIndex::iterator it) {
  books_[it->second.stock_locate].DeleteOrder(it->second);
  orders_.erase(it);
}

std::optional<FeedHandler::OrderIndex::iterator> FeedHandler::FindOrder(
    const OrderId order_id) {
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    std::cerr << "Error: Order not found, ID: " << order_id << std::endl;
    return std::nullopt;
  }
  return it;
}
