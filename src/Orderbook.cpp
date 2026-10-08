#include "../include/Orderbook.h"
#include "../include/Side.h"
#include "../include/Order.h"
#include "../include/Handle.h"
#include "../include/Types.h"
#include <list>
#include <map>
#include <iostream>

void Orderbook::AddOrder(Order order) {
    if (order.side == Side::Buy) {HelperAddOrder(bids, order);}
    if (order.side == Side::Sell) {HelperAddOrder(asks, order);}
};
template <typename T>
void Orderbook::HelperAddOrder(T& map, Order order) {
    Handle handle;

    auto pair = map.insert(std::pair<Price, PriceLevelOrders>(order.price, PriceLevelOrders{}));
    auto it = pair.first;
    auto location = it->second.insert(it->second.end(), order);

    handle.price = order.price;
    handle.side = order.side;
    handle.location = location;
    orders.insert(std::pair<OrderId, Handle> (order.orderId, handle));
}
void Orderbook::DeleteOrder(OrderId orderId) {
    auto it = orders.find(orderId);
    if (it == orders.end()) {
        std::cerr << "Error: Order not found!" << orderId << "\n";
        return;
    };
    if (it->second.side == Side::Buy) {HelperDeleteOrder(bids, orderId, it);}
    if (it->second.side == Side::Sell) {HelperDeleteOrder(asks, orderId, it);}
};
template <typename T>
void Orderbook::HelperDeleteOrder(T& map, OrderId orderId, auto it) {
    auto const list = map.find(it->second.price);
    PriceLevelOrders &levelOrder = list->second;
    levelOrder.erase(it->second.location);
    if (levelOrder.empty()) {map.erase(it->second.price);}
    orders.erase(orderId);
};

void Orderbook::ReplaceOrder(Order order, OrderId orderId) {
    DeleteOrder(orderId);
    AddOrder(order);
};
void Orderbook::OrderExecuted(OrderId orderId, Quantity quantity) {
    auto const it = orders.find(orderId);
    if (it == orders.end()) {
        std::cerr << "Error: Order not found!\n";
        return;
    };
    auto order = it->second.location;
    order->quantity = order->quantity - quantity;
    if (order->quantity == 0) {
        DeleteOrder(orderId);
    };
}