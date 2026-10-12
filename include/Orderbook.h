#ifndef ORDERBOOK_ORDERBOOK_H
#define ORDERBOOK_ORDERBOOK_H
#include <map>
#include <string>
#include <unordered_map>
#include "Types.h"
#include "Handle.h"
#include "Order.h"

class Orderbook {
public:
    std::string name;

    Orderbook()= default;

    explicit Orderbook(const std::string &name) {
        this->name = name;
    }

    std::map<Price, PriceLevelOrders, std::greater<>> bids;
    std::map<Price, PriceLevelOrders, std::less<>> asks;

    void AddOrder(const Order &order, std::unordered_map<OrderId, Handle>& orders);
    template <typename T>
    void HelperAddOrder(T& map, Order order, std::unordered_map<OrderId, Handle>& orders);
    void DeleteOrder(std::unordered_map<OrderId, Handle>::iterator it, std::unordered_map<OrderId, Handle>& orders);
    template <typename T>
    void HelperDeleteOrder(T& map, std::unordered_map<OrderId, Handle>::iterator it, std::unordered_map<OrderId, Handle>& orders);
    void ReplaceOrder(const Order &order, std::unordered_map<OrderId, Handle>::iterator it, std::unordered_map<OrderId, Handle>& orders);
    void OrderExecuted(std::unordered_map<OrderId, Handle>::iterator it, Quantity quantity, std::unordered_map<OrderId, Handle>& orders);
};

#endif //ORDERBOOK_ORDERBOOK_H
