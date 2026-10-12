#ifndef ITCH_PROCESSOR_HANDLE_H_
#define ITCH_PROCESSOR_HANDLE_H_
#include "side.h"
#include "types.h"

struct Handle {
  Price price;
  Side side;
  PriceLevel::iterator location;
  StockLocate stock_locate;
};

#endif  // ITCH_PROCESSOR_HANDLE_H_
