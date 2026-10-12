#ifndef ITCH_PROCESSOR_TEST_ITCH_BUILDER_H_
#define ITCH_PROCESSOR_TEST_ITCH_BUILDER_H_
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "common/types.h"

// Encodes ITCH 5.0 messages for tests. Offsets and lengths follow the Nasdaq
// TotalView-ITCH 5.0 spec; tracking number and timestamp are left zero.

namespace itch_test {

struct StockDirectory {
  StockLocate stock_locate = 1;
  std::string symbol;
};

// Encoded as 'F' (with MPID attribution) when attributed is set, else 'A'.
struct AddOrder {
  StockLocate stock_locate = 1;
  OrderId order_id = 0;
  char side = 'B';
  Quantity quantity = 0;
  Price price = 0;
  bool attributed = false;
};

struct OrderExecuted {
  StockLocate stock_locate = 1;
  OrderId order_id = 0;
  Quantity quantity = 0;
};

struct OrderExecutedWithPrice {
  StockLocate stock_locate = 1;
  OrderId order_id = 0;
  Quantity quantity = 0;
  Price price = 0;
};

struct OrderCancel {
  StockLocate stock_locate = 1;
  OrderId order_id = 0;
  Quantity quantity = 0;
};

struct OrderDelete {
  StockLocate stock_locate = 1;
  OrderId order_id = 0;
};

struct OrderReplace {
  StockLocate stock_locate = 1;
  OrderId original_order_id = 0;
  OrderId new_order_id = 0;
  Quantity quantity = 0;
  Price price = 0;
};

namespace internal {

// Writes the common header (type, stock locate) and big-endian fields into a
// zeroed message of fixed length.
class Writer {
 public:
  Writer(const char type, const std::size_t length,
         const StockLocate stock_locate)
      : bytes_(length) {
    bytes_[0] = static_cast<uint8_t>(type);
    Put(1, stock_locate);
  }

  template <typename T>
  Writer& Put(const std::size_t offset, const T value) {
    for (std::size_t i = 0; i < sizeof(T); ++i) {
      bytes_[offset + i] =
          static_cast<uint8_t>(value >> (8 * (sizeof(T) - 1 - i)));
    }
    return *this;
  }

  // Alpha fields are left-justified and padded with spaces.
  Writer& PutAlpha(const std::size_t offset, const std::string_view value,
                   const std::size_t width) {
    for (std::size_t i = 0; i < width; ++i) {
      bytes_[offset + i] = i < value.size() ? value[i] : ' ';
    }
    return *this;
  }

  const std::vector<uint8_t>& bytes() const { return bytes_; }

 private:
  std::vector<uint8_t> bytes_;
};

}  // namespace internal

inline std::vector<uint8_t> Encode(const StockDirectory& m) {
  return internal::Writer('R', 39, m.stock_locate)
      .PutAlpha(11, m.symbol, 8)
      .bytes();
}

inline std::vector<uint8_t> Encode(const AddOrder& m) {
  return internal::Writer(m.attributed ? 'F' : 'A', m.attributed ? 40 : 36,
                          m.stock_locate)
      .Put(11, m.order_id)
      .Put(19, static_cast<uint8_t>(m.side))
      .Put(20, m.quantity)
      .PutAlpha(24, "TEST", 8)
      .Put(32, m.price)
      .bytes();
}

inline std::vector<uint8_t> Encode(const OrderExecuted& m) {
  return internal::Writer('E', 31, m.stock_locate)
      .Put(11, m.order_id)
      .Put(19, m.quantity)
      .bytes();
}

inline std::vector<uint8_t> Encode(const OrderExecutedWithPrice& m) {
  return internal::Writer('C', 36, m.stock_locate)
      .Put(11, m.order_id)
      .Put(19, m.quantity)
      .Put(31, static_cast<uint8_t>('Y'))
      .Put(32, m.price)
      .bytes();
}

inline std::vector<uint8_t> Encode(const OrderCancel& m) {
  return internal::Writer('X', 23, m.stock_locate)
      .Put(11, m.order_id)
      .Put(19, m.quantity)
      .bytes();
}

inline std::vector<uint8_t> Encode(const OrderDelete& m) {
  return internal::Writer('D', 19, m.stock_locate).Put(11, m.order_id).bytes();
}

inline std::vector<uint8_t> Encode(const OrderReplace& m) {
  return internal::Writer('U', 35, m.stock_locate)
      .Put(11, m.original_order_id)
      .Put(19, m.new_order_id)
      .Put(27, m.quantity)
      .Put(31, m.price)
      .bytes();
}

// Appends a message with its 2-byte big-endian length prefix, as in a
// BinaryFILE stream.
inline void AppendFramed(std::vector<uint8_t>& out,
                         const std::vector<uint8_t>& message) {
  out.push_back(static_cast<uint8_t>(message.size() >> 8));
  out.push_back(static_cast<uint8_t>(message.size()));
  out.insert(out.end(), message.begin(), message.end());
}

template <typename... Messages>
std::vector<uint8_t> Feed(const Messages&... messages) {
  std::vector<uint8_t> out;
  (AppendFramed(out, Encode(messages)), ...);
  return out;
}

}  // namespace itch_test

#endif  // ITCH_PROCESSOR_TEST_ITCH_BUILDER_H_
