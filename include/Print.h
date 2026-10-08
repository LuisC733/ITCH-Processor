#ifndef ORDERBOOK_PRINT_H
#define ORDERBOOK_PRINT_H
#include <iostream>
#include "Types.h"

template <typename T>
void Print(T& map) {
    for (auto const& i : map) {
        std::cout << i.first<< ", ";
        Quantity result = 0;
        for (auto const& order : i.second) {
            result += order.quantity;
        }
        std::cout << result << std::endl;
    }
}

#endif //ORDERBOOK_PRINT_H
