#include "orderbook/orderbook.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "book_util.h"
#include "common/side.h"
#include "common/types.h"
#include "orderbook/handle.h"
#include "orderbook/order.h"

namespace {

using ::itch_test::Depth;
using ::itch_test::QueueIds;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Pair;

constexpr StockLocate kLocate = 1;

Handle AddTo(Orderbook& book, const OrderId order_id, const Side side,
             const Quantity quantity, const Price price) {
  const Order order{.price = price,
                    .quantity = quantity,
                    .order_id = order_id,
                    .side = side,
                    .stock_locate = kLocate};
  return Handle{price, side, book.AddOrder(order), kLocate};
}

class OrderbookTest : public testing::Test {
 protected:
  Handle Add(const OrderId order_id, const Side side, const Quantity quantity,
             const Price price) {
    return AddTo(book_, order_id, side, quantity, price);
  }

  void TearDown() override { book_.CheckInvariants(); }

  Orderbook book_{"TEST"};
};

TEST_F(OrderbookTest, KeepsName) { EXPECT_EQ(book_.name(), "TEST"); }

TEST_F(OrderbookTest, SortsBidsDescendingAndAsksAscending) {
  Add(1, Side::kBuy, 10, 998);
  Add(2, Side::kBuy, 20, 1000);
  Add(3, Side::kBuy, 30, 999);
  Add(4, Side::kSell, 40, 1003);
  Add(5, Side::kSell, 50, 1001);

  EXPECT_THAT(Depth(book_.bids()),
              ElementsAre(Pair(1000, 20), Pair(999, 30), Pair(998, 10)));
  EXPECT_THAT(Depth(book_.asks()), ElementsAre(Pair(1001, 50), Pair(1003, 40)));
}

TEST_F(OrderbookTest, QueuesOrdersFifoWithinLevel) {
  Add(1, Side::kBuy, 10, 1000);
  Add(2, Side::kBuy, 20, 1000);
  Add(3, Side::kBuy, 30, 1000);

  EXPECT_THAT(QueueIds(book_.bids().at(1000)), ElementsAre(1, 2, 3));
  EXPECT_THAT(Depth(book_.bids()), ElementsAre(Pair(1000, 60)));
}

TEST_F(OrderbookTest, PartialReduceKeepsQueuePosition) {
  const Handle first = Add(1, Side::kSell, 100, 1000);
  Add(2, Side::kSell, 100, 1000);

  EXPECT_FALSE(book_.ReduceQuantity(first, 30));

  EXPECT_EQ(first.location->quantity, 70u);
  EXPECT_THAT(QueueIds(book_.asks().at(1000)), ElementsAre(1, 2));
}

TEST_F(OrderbookTest, FullReduceRemovesOrderAndKeepsOthers) {
  const Handle first = Add(1, Side::kSell, 100, 1000);
  Add(2, Side::kSell, 50, 1000);

  EXPECT_TRUE(book_.ReduceQuantity(first, 100));

  EXPECT_THAT(QueueIds(book_.asks().at(1000)), ElementsAre(2));
}

TEST_F(OrderbookTest, FullReduceOfLastOrderRemovesLevel) {
  const Handle only = Add(1, Side::kBuy, 100, 1000);
  Add(2, Side::kBuy, 50, 999);

  EXPECT_TRUE(book_.ReduceQuantity(only, 100));

  EXPECT_THAT(Depth(book_.bids()), ElementsAre(Pair(999, 50)));
}

TEST_F(OrderbookTest, DeleteFromMiddleKeepsQueueOrder) {
  Add(1, Side::kBuy, 10, 1000);
  const Handle middle = Add(2, Side::kBuy, 20, 1000);
  Add(3, Side::kBuy, 30, 1000);

  book_.DeleteOrder(middle);

  EXPECT_THAT(QueueIds(book_.bids().at(1000)), ElementsAre(1, 3));
}

TEST_F(OrderbookTest, DeleteOfLastOrderRemovesLevel) {
  const Handle only = Add(1, Side::kSell, 10, 1000);

  book_.DeleteOrder(only);

  EXPECT_THAT(book_.asks(), IsEmpty());
}

// Invariant checks abort in Debug builds; in Release the statements just run.

TEST(OrderbookDeathTest, CrossingOrderAborts) {
  Orderbook book{"TEST"};
  AddTo(book, 1, Side::kBuy, 10, 1000);
  EXPECT_DEBUG_DEATH(AddTo(book, 2, Side::kSell, 10, 999),
                     "best_bid < best_ask");
}

TEST(OrderbookDeathTest, LockingOrderAborts) {
  Orderbook book{"TEST"};
  AddTo(book, 1, Side::kSell, 10, 1000);
  EXPECT_DEBUG_DEATH(AddTo(book, 2, Side::kBuy, 10, 1000),
                     "best_bid < best_ask");
}

TEST(OrderbookDeathTest, ReducingBelowZeroAborts) {
  Orderbook book{"TEST"};
  const Handle handle = AddTo(book, 1, Side::kBuy, 100, 1000);
  EXPECT_DEBUG_DEATH(book.ReduceQuantity(handle, 101),
                     "remaining=100 reduce_by=101");
}

TEST(OrderbookDeathTest, ZeroQuantityOrderAborts) {
  Orderbook book{"TEST"};
  EXPECT_DEBUG_DEATH(AddTo(book, 1, Side::kBuy, 0, 1000), "order.quantity > 0");
}

}  // namespace
