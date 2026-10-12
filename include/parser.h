#ifndef ITCH_PROCESSOR_PARSER_H_
#define ITCH_PROCESSOR_PARSER_H_
#include <bit>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <utility>

#include "order.h"
#include "side.h"
#include "types.h"

static_assert(std::endian::native == std::endian::little,
              "ReadBigEndian assumes a little-endian host");

// Reads a big-endian unsigned integer from an unaligned position.
template <typename T>
inline T ReadBigEndian(const uint8_t* p) {
  static_assert(std::is_unsigned_v<T>);
  static_assert(sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
  T value;
  memcpy(&value, p, sizeof(value));
  if constexpr (sizeof(T) == 2) {
    return __builtin_bswap16(value);
  } else if constexpr (sizeof(T) == 4) {
    return __builtin_bswap32(value);
  } else {
    return __builtin_bswap64(value);
  }
}

// All parsers take a pointer to the message type byte.

inline Order ParseAddOrder(const uint8_t* msg) {
  Order order{};
  order.stock_locate = ReadBigEndian<StockLocate>(msg + 1);
  order.order_id = ReadBigEndian<OrderId>(msg + 11);
  order.side = msg[19] == 'B' ? Side::kBuy : Side::kSell;
  order.quantity = ReadBigEndian<Quantity>(msg + 20);
  order.price = ReadBigEndian<Price>(msg + 32);
  return order;
}

inline OrderId ParseOrderId(const uint8_t* msg) {
  return ReadBigEndian<OrderId>(msg + 11);
}

inline Quantity ParseOrderQuantity(const uint8_t* msg) {
  return ReadBigEndian<Quantity>(msg + 19);
}

inline Order ParseReplaceOrder(const uint8_t* msg) {
  Order order{};
  order.stock_locate = ReadBigEndian<StockLocate>(msg + 1);
  order.order_id = ReadBigEndian<OrderId>(msg + 19);
  order.quantity = ReadBigEndian<Quantity>(msg + 27);
  order.price = ReadBigEndian<Price>(msg + 31);
  return order;
}

inline std::pair<StockLocate, std::string> ParseStockDirectory(
    const uint8_t* msg) {
  const StockLocate stock_locate = ReadBigEndian<StockLocate>(msg + 1);
  std::string stock_name(msg + 11, msg + 19);
  stock_name.erase(stock_name.find_last_not_of(' ') + 1);
  return {stock_locate, stock_name};
}

#endif  // ITCH_PROCESSOR_PARSER_H_
