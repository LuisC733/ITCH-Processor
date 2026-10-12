#pragma once
#ifndef ORDERBOOK_ORDER_H
#define ORDERBOOK_ORDER_H
#include "Types.h"
#include "Side.h"

struct Order {
    Price price;
    Quantity quantity;
    OrderId orderId;
    Side side;
    StockLocate stockLocate;
};

#endif //ORDERBOOK_ORDER_H
