#include <iostream>
#include <map>
#include "Parser.h"
#include "Orderbook.h"
#include "Reader.h"

int main() {
    Orderbook orderbook{};
    const std::vector<uint8_t> src = readFile("assets/20200130.BX_ITCH_50");
    size_t offset = 0;
    while (offset < src.size()) {
        uint16_t dest = {};
        memcpy(&dest, src.data() + offset, sizeof(dest));
        uint16_t length = __builtin_bswap16(dest);
        offset += 2;
        switch (src[offset]) {
            case 'A': [[fallthrough]];
            case 'F':
                orderbook.AddOrder(parseAddOrder(src, offset));
                break;
            case 'D':
                orderbook.DeleteOrder(parseOrderId(src, offset));
                break;
            case 'E': [[fallthrough]];
            case 'C': [[fallthrough]];
            case 'X':
                orderbook.OrderExecuted(parseOrderId(src, offset), parseOrderQuantity(src, offset));
                break;
            {
                case 'U':
                OrderId id = parseOrderId(src, offset);
                auto const it = orderbook.orders.find(id);
                if (it == orderbook.orders.end()) {
                    std::cerr << "Error: Order not found!\n";
                    break;
                };
                const Side side = it->second.side;
                Order order = parseReplaceOrder(src, offset);
                order.side = side;
                orderbook.ReplaceOrder(order, id);
                break;
            }
            default:
                break;
        }
        offset += length;
    }

    return 0;
}
