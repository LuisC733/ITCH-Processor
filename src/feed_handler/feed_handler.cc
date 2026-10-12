#include "feed_handler/feed_handler.h"

#include <iostream>

#include "orderbook/order.h"
#include "parser/parser.h"

FeedHandler::FeedHandler() : books_(kMaxLocate) {}

void FeedHandler::Process(const std::vector<uint8_t>& src) {
  size_t offset = 0;
  while (offset + 2 <= src.size()) {
    const uint16_t length = ReadBigEndian<uint16_t>(src.data() + offset);
    offset += 2;
    if (length == 0 || offset + length > src.size()) {
      std::cerr << "Error: Truncated message at offset: " << offset
                << std::endl;
      break;
    }
    ProcessMessage(src.data() + offset);
    offset += length;
  }
}

const Orderbook& FeedHandler::book(const StockLocate stock_locate) const {
  return books_[stock_locate];
}

void FeedHandler::ProcessMessage(const uint8_t* msg) {
  switch (msg[0]) {
    case 'R':
      OnStockDirectory(msg);
      break;
    case 'A':
    case 'F':
      OnAddOrder(msg);
      break;
    case 'D':
      OnDeleteOrder(msg);
      break;
    case 'E':
    case 'C':
    case 'X':
      OnOrderExecuted(msg);
      break;
    case 'U':
      OnReplaceOrder(msg);
      break;
    default:
      break;
  }
}

void FeedHandler::OnStockDirectory(const uint8_t* msg) {
  const auto [stock_locate, name] = ParseStockDirectory(msg);
  books_[stock_locate] = Orderbook{name};
}

void FeedHandler::OnAddOrder(const uint8_t* msg) {
  InsertOrder(ParseAddOrder(msg));
}

void FeedHandler::OnDeleteOrder(const uint8_t* msg) {
  const auto found = FindOrder(ParseOrderId(msg));
  if (!found) {
    return;
  }
  RemoveOrder(*found);
}

void FeedHandler::OnOrderExecuted(const uint8_t* msg) {
  const auto found = FindOrder(ParseOrderId(msg));
  if (!found) {
    return;
  }
  const auto it = *found;
  if (books_[it->second.stock_locate].ReduceQuantity(it->second,
                                                     ParseOrderQuantity(msg))) {
    orders_.erase(it);
  }
}

void FeedHandler::OnReplaceOrder(const uint8_t* msg) {
  const auto found = FindOrder(ParseOrderId(msg));
  if (!found) {
    return;
  }
  Order order = ParseReplaceOrder(msg);
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
