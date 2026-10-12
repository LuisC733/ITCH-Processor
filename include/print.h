#ifndef ITCH_PROCESSOR_PRINT_H_
#define ITCH_PROCESSOR_PRINT_H_
#include <iostream>

#include "types.h"

template <typename T>
void Print(T& map) {
  for (auto const& i : map) {
    std::cout << i.first << ", ";
    Quantity result = 0;
    for (auto const& order : i.second) {
      result += order.quantity;
    }
    std::cout << result << std::endl;
  }
}

#endif  // ITCH_PROCESSOR_PRINT_H_
