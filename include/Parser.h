#ifndef ITCH_PROCESSOR_PARSER_H
#define ITCH_PROCESSOR_PARSER_H
#include <vector>
#include "Order.h"
#include "Orderbook.h"

inline Order parseAddOrder(const std::vector<uint8_t> &src, const size_t offset) {
    Order order{};
    OrderId orderId;
    Quantity quantity;
    Price price;

    memcpy(&orderId, src.data() + offset + 11, sizeof(orderId));
    order.orderId  = __builtin_bswap64(orderId);
    if (src[offset + 19] == 'B') {order.side = Side::Buy;}
    else{order.side = Side::Sell;}
    memcpy(&quantity, src.data() + offset + 20, sizeof(quantity));
    order.quantity = __builtin_bswap32(quantity);
    memcpy(&price, src.data() + offset + 32, sizeof(price));
    order.price = __builtin_bswap32(price);

    return order;
};
inline OrderId parseOrderId(const std::vector<uint8_t> &src, const size_t offset) {
    OrderId orderId;
    memcpy(&orderId, src.data() + offset + 11, sizeof(orderId));
    return __builtin_bswap64(orderId);
};

inline Quantity parseOrderQuantity(const std::vector<uint8_t> &src, const size_t offset) {
    Quantity quantity;
    memcpy(&quantity, src.data() + offset + 19, sizeof(quantity));
    return __builtin_bswap32(quantity);
}

inline Order parseReplaceOrder(const std::vector<uint8_t> &src, const size_t offset) {
    Order order{};
    OrderId orderId;
    Quantity quantity;
    Price price;

    memcpy(&orderId, src.data() + offset + 19, sizeof(orderId));
    order.orderId  = __builtin_bswap64(orderId);
    memcpy(&quantity, src.data() + offset + 27, sizeof(quantity));
    order.quantity = __builtin_bswap32(quantity);
    memcpy(&price, src.data() + offset + 31, sizeof(price));
    order.price = __builtin_bswap32(price);

    return order;
};

#endif //ITCH_PROCESSOR_PARSER_H
