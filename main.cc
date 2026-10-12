#include <iostream>
#include <map>
#include <vector>

#include "orderbook.h"
#include "parser.h"
#include "reader.h"

int main() {
  constexpr int kMaxLocate = 65536;
  const std::vector<uint8_t> src = ReadFile("assets/20200130.BX_ITCH_50");
  std::vector<Orderbook> stocks(kMaxLocate);
  std::unordered_map<OrderId, Handle> orders;
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
    switch (src[offset]) {
      {
        case 'R':
          const auto [fst, snd] = ParseStockDirectory(src, offset);
          const Orderbook orderbook{snd};
          stocks[fst] = orderbook;
          break;
      }
      {
        case 'A':
          [[fallthrough]];
        case 'F':
          const Order order = ParseAddOrder(src, offset);
          stocks[order.stock_locate].AddOrder(order, orders);
          break;
      }
      {
        case 'D':
          OrderId id = ParseOrderId(src, offset);
          auto it = orders.find(id);
          if (it == orders.end()) {
            std::cerr << "Error: Order not found, ID: " << id << std::endl;
            break;
          }
          stocks[it->second.stock_locate].DeleteOrder(it, orders);
          break;
      }
      {
        case 'E':
          [[fallthrough]];
        case 'C':
          [[fallthrough]];
        case 'X':
          OrderId id = ParseOrderId(src, offset);
          auto it = orders.find(id);
          if (it == orders.end()) {
            std::cerr << "Error: Order not found, ID: " << id << std::endl;
            break;
          }
          stocks[it->second.stock_locate].OrderExecuted(
              it, ParseOrderQuantity(src, offset), orders);
          break;
      }
      {
        case 'U':
          OrderId id = ParseOrderId(src, offset);
          auto it = orders.find(id);
          if (it == orders.end()) {
            std::cerr << "Error: Order not found, ID: " << id << std::endl;
            break;
          }
          Order order = ParseReplaceOrder(src, offset);
          order.side = it->second.side;
          stocks[it->second.stock_locate].ReplaceOrder(order, it, orders);
          break;
      }
      default:
        break;
    }
    offset += length;
  }
  return 0;
}
