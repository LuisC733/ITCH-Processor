#ifndef ITCH_PROCESSOR_PARSER_H_
#define ITCH_PROCESSOR_PARSER_H_
#include <cstring>
#include <utility>
#include <vector>

#include "order.h"
#include "orderbook.h"

inline Order ParseAddOrder(const std::vector<uint8_t>& src,
                           const size_t offset) {
  Order order{};
  OrderId order_id;
  Quantity quantity;
  Price price;
  StockLocate stock_locate;

  memcpy(&stock_locate, src.data() + offset + 1, sizeof(stock_locate));
  order.stock_locate = __builtin_bswap16(stock_locate);
  memcpy(&order_id, src.data() + offset + 11, sizeof(order_id));
  order.order_id = __builtin_bswap64(order_id);
  if (src[offset + 19] == 'B') {
    order.side = Side::kBuy;
  } else {
    order.side = Side::kSell;
  }
  memcpy(&quantity, src.data() + offset + 20, sizeof(quantity));
  order.quantity = __builtin_bswap32(quantity);
  memcpy(&price, src.data() + offset + 32, sizeof(price));
  order.price = __builtin_bswap32(price);

  return order;
};
inline OrderId ParseOrderId(const std::vector<uint8_t>& src,
                            const size_t offset) {
  OrderId order_id;
  memcpy(&order_id, src.data() + offset + 11, sizeof(order_id));
  return __builtin_bswap64(order_id);
};

inline Quantity ParseOrderQuantity(const std::vector<uint8_t>& src,
                                   const size_t offset) {
  Quantity quantity;
  memcpy(&quantity, src.data() + offset + 19, sizeof(quantity));
  return __builtin_bswap32(quantity);
}

inline Order ParseReplaceOrder(const std::vector<uint8_t>& src,
                               const size_t offset) {
  Order order{};
  OrderId order_id;
  Quantity quantity;
  Price price;
  StockLocate stock_locate;

  memcpy(&stock_locate, src.data() + offset + 1, sizeof(stock_locate));
  order.stock_locate = __builtin_bswap16(stock_locate);
  memcpy(&order_id, src.data() + offset + 19, sizeof(order_id));
  order.order_id = __builtin_bswap64(order_id);
  memcpy(&quantity, src.data() + offset + 27, sizeof(quantity));
  order.quantity = __builtin_bswap32(quantity);
  memcpy(&price, src.data() + offset + 31, sizeof(price));
  order.price = __builtin_bswap32(price);

  return order;
};

inline std::pair<StockLocate, std::string> ParseStockDirectory(
    const std::vector<uint8_t>& src, const size_t offset) {
  StockLocate stock_locate;

  memcpy(&stock_locate, src.data() + offset + 1, sizeof(stock_locate));
  stock_locate = __builtin_bswap16(stock_locate);
  std::string stock_name(src.data() + offset + 11, src.data() + offset + 19);
  stock_name.erase(stock_name.find_last_not_of(' ') + 1);

  auto pair = std::make_pair(stock_locate, stock_name);
  return pair;
};

#endif  // ITCH_PROCESSOR_PARSER_H_
