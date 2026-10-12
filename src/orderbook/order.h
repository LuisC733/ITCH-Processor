#ifndef ITCH_PROCESSOR_ORDERBOOK_ORDER_H_
#define ITCH_PROCESSOR_ORDERBOOK_ORDER_H_
#include "common/side.h"
#include "common/types.h"

struct Order {
  Price price;
  Quantity quantity;
  OrderId order_id;
  Side side;
  StockLocate stock_locate;
};

#endif  // ITCH_PROCESSOR_ORDERBOOK_ORDER_H_
