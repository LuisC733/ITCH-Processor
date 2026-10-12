#include "../include/Orderbook.h"
#include "../include/Side.h"
#include "../include/Order.h"
#include "../include/Handle.h"
#include "../include/Types.h"
#include <list>
#include <map>

void Orderbook::AddOrder(const Order &order, std::unordered_map<OrderId, Handle>& orders) {
    if (order.side == Side::Buy) {HelperAddOrder(bids, order, orders);}
    else {HelperAddOrder(asks, order, orders);}
};
template <typename T>
void Orderbook::HelperAddOrder(T& map, Order order, std::unordered_map<OrderId, Handle>& orders) {
    Handle handle;

    auto pair = map.insert(std::pair(order.price, PriceLevelOrders{}));
    auto it = pair.first;
    auto location = it->second.insert(it->second.end(), order);

    handle.price = order.price;
    handle.side = order.side;
    handle.location = location;
    handle.stockLocate = order.stockLocate;
    orders.insert(std::pair (order.orderId, handle));
}
void Orderbook::DeleteOrder(const std::unordered_map<OrderId, Handle>::iterator it, std::unordered_map<OrderId, Handle>& orders) {
    if (it->second.side == Side::Buy) {HelperDeleteOrder(bids, it, orders);}
    else {HelperDeleteOrder(asks, it, orders);}
};
template <typename T>
void Orderbook::HelperDeleteOrder(T& map, const std::unordered_map<OrderId, Handle>::iterator it, std::unordered_map<OrderId, Handle>& orders) {
    auto const list = map.find(it->second.price);
    PriceLevelOrders &levelOrder = list->second;
    levelOrder.erase(it->second.location);
    if (levelOrder.empty()) {map.erase(list);}
    orders.erase(it);
};

void Orderbook::ReplaceOrder(const Order &order, const std::unordered_map<OrderId, Handle>::iterator it, std::unordered_map<OrderId, Handle>& orders) {
    DeleteOrder(it, orders);
    AddOrder(order, orders);
};
void Orderbook::OrderExecuted(const std::unordered_map<OrderId, Handle>::iterator it, const Quantity quantity, std::unordered_map<OrderId, Handle>& orders) {
    const auto order = it->second.location;
    order->quantity = order->quantity - quantity;
    if (order->quantity == 0) {
        DeleteOrder(it, orders);
    };
}