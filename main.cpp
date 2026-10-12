#include <iostream>
#include <map>
#include "Parser.h"
#include "Orderbook.h"
#include "Reader.h"
#include <vector>

int main() {
    constexpr int MAX_LOCATE = 65536;
    const std::vector<uint8_t> src = readFile("assets/20200130.BX_ITCH_50");
    std::vector<Orderbook> stocks(MAX_LOCATE);
    std::unordered_map<OrderId, Handle> orders;
    size_t offset = 0;
    while (offset + 2 <= src.size()) {
        uint16_t dest = {};
        memcpy(&dest, src.data() + offset, sizeof(dest));
        const uint16_t length = __builtin_bswap16(dest);
        offset += 2;
        if (length == 0 || offset + length > src.size()) {
            std::cerr << "Error: Truncated message at offset: " << offset << std::endl;
            break;
        }
        switch (src[offset]) {
            {
                case 'R':
                const auto [fst, snd] = parseStockDirectory(src, offset);
                const Orderbook orderbook{snd};
                stocks[fst] = orderbook;
                break;
            }
            {
                case 'A': [[fallthrough]];
                case 'F':
                const Order order = parseAddOrder(src, offset);
                stocks[order.stockLocate].AddOrder(order, orders);
                break;
            }
            {
                case 'D':
                OrderId id = parseOrderId(src, offset);
                auto it = orders.find(id);
                if (it == orders.end()) {
                    std::cerr << "Error: Order not found, ID: " << id << std::endl;
                    break;
                }
                stocks[it->second.stockLocate].DeleteOrder(it, orders);
                break;
            }
            {
                case 'E': [[fallthrough]];
                case 'C': [[fallthrough]];
                case 'X':
                OrderId id = parseOrderId(src, offset);
                auto it = orders.find(id);
                if (it == orders.end()) {
                    std::cerr << "Error: Order not found, ID: " << id << std::endl;
                    break;
                }
                stocks[it->second.stockLocate].OrderExecuted(it, parseOrderQuantity(src, offset), orders);
                break;
            }
            {
                case 'U':
                OrderId id = parseOrderId(src, offset);
                auto it = orders.find(id);
                if (it == orders.end()) {
                    std::cerr << "Error: Order not found, ID: " << id << std::endl;
                    break;
                }
                Order order = parseReplaceOrder(src, offset);
                order.side = it->second.side;
                stocks[it->second.stockLocate].ReplaceOrder(order, it, orders);
                break;
            }
            default:
                break;
        }
        offset += length;
    }
    return 0;
}
