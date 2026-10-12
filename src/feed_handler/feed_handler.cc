#include "feed_handler/feed_handler.h"

#include <cstddef>
#include <iostream>

#include "common/check.h"
#include "orderbook/order.h"
#include "parser/parser.h"

namespace {

template <typename T>
std::size_t CountOrders(const T& map) {
  std::size_t count = 0;
  for (const auto& [price, price_level] : map) {
    count += price_level.size();
  }
  return count;
}

}  // namespace

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
#ifndef NDEBUG
  CheckInvariants();
#endif
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
  // Replacing a book that still holds orders would leave dangling handles.
  ITCH_DCHECK(books_[stock_locate].bids().empty() &&
              books_[stock_locate].asks().empty())
      << "stock_locate=" << stock_locate << " name=" << name;
  books_[stock_locate] = Orderbook{name};
}

void FeedHandler::OnAddOrder(const uint8_t* msg) {
  InsertOrder(ParseAddOrder(msg));
}

void FeedHandler::OnDeleteOrder(const uint8_t* msg) {
  const auto found = FindOrder(msg);
  if (!found) {
    return;
  }
  RemoveOrder(*found);
}

void FeedHandler::OnOrderExecuted(const uint8_t* msg) {
  const auto found = FindOrder(msg);
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
  const auto found = FindOrder(msg);
  if (!found) {
    return;
  }
  Order order = ParseReplaceOrder(msg);
  order.side = (*found)->second.side;
  RemoveOrder(*found);
  InsertOrder(order);
}

void FeedHandler::InsertOrder(const Order& order) {
  ITCH_DCHECK(!orders_.contains(order.order_id))
      << "duplicate order_id=" << order.order_id;
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
    const uint8_t* msg) {
  const OrderId order_id = ParseOrderId(msg);
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    std::cerr << "Error: Order not found, ID: " << order_id << std::endl;
    return std::nullopt;
  }
  ITCH_DCHECK(it->second.stock_locate == ParseStockLocate(msg))
      << "order_id=" << order_id << " indexed under " << it->second.stock_locate
      << ", message for " << ParseStockLocate(msg);
  return it;
}

void FeedHandler::CheckInvariants() const {
  // Every indexed order must point at a live order that matches its handle.
  for (const auto& [order_id, handle] : orders_) {
    const Order& order = *handle.location;
    ITCH_DCHECK(order.order_id == order_id && order.price == handle.price &&
                order.side == handle.side &&
                order.stock_locate == handle.stock_locate)
        << "order_id=" << order_id;
  }
  // And every order in a book must be indexed, so the counts have to agree.
  std::size_t book_orders = 0;
  for (const Orderbook& book : books_) {
    book.CheckInvariants();
    book_orders += CountOrders(book.bids()) + CountOrders(book.asks());
  }
  ITCH_DCHECK(book_orders == orders_.size())
      << "orders in books=" << book_orders << " indexed=" << orders_.size();
}
