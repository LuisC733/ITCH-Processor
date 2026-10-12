#include "feed_handler/feed_handler.h"

#include <cstdint>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "book_util.h"
#include "itch_builder.h"

namespace {

using ::itch_test::AddOrder;
using ::itch_test::AppendFramed;
using ::itch_test::Depth;
using ::itch_test::Feed;
using ::itch_test::OrderCancel;
using ::itch_test::OrderDelete;
using ::itch_test::OrderExecuted;
using ::itch_test::OrderExecutedWithPrice;
using ::itch_test::OrderReplace;
using ::itch_test::QueueIds;
using ::itch_test::StockDirectory;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Pair;

class FeedHandlerTest : public testing::Test {
 protected:
  template <typename... Messages>
  void Process(const Messages&... messages) {
    handler_.Process(Feed(messages...));
  }

  const Orderbook& book(const StockLocate stock_locate = 1) const {
    return handler_.book(stock_locate);
  }

  // Every scenario must leave the books and the order index consistent.
  void TearDown() override { handler_.CheckInvariants(); }

  FeedHandler handler_;
};

TEST_F(FeedHandlerTest, StockDirectoryNamesBook) {
  Process(StockDirectory{.stock_locate = 3, .symbol = "MSFT"});

  EXPECT_EQ(book(3).name(), "MSFT");
}

TEST_F(FeedHandlerTest, AddOrdersLandOnTheirSide) {
  Process(AddOrder{.order_id = 1, .side = 'B', .quantity = 100, .price = 999},
          AddOrder{.order_id = 2,
                   .side = 'S',
                   .quantity = 50,
                   .price = 1001,
                   .attributed = true});

  EXPECT_THAT(Depth(book().bids()), ElementsAre(Pair(999, 100)));
  EXPECT_THAT(Depth(book().asks()), ElementsAre(Pair(1001, 50)));
}

TEST_F(FeedHandlerTest, RoutesMessagesByStockLocate) {
  Process(StockDirectory{.stock_locate = 1, .symbol = "AAPL"},
          StockDirectory{.stock_locate = 2, .symbol = "MSFT"},
          AddOrder{
              .stock_locate = 1, .order_id = 1, .quantity = 100, .price = 1000},
          AddOrder{
              .stock_locate = 2, .order_id = 2, .quantity = 200, .price = 2000},
          OrderExecuted{.stock_locate = 2, .order_id = 2, .quantity = 50});

  EXPECT_THAT(Depth(book(1).bids()), ElementsAre(Pair(1000, 100)));
  EXPECT_THAT(Depth(book(2).bids()), ElementsAre(Pair(2000, 150)));
}

TEST_F(FeedHandlerTest, PartialExecutionReducesQuantity) {
  Process(AddOrder{.order_id = 1, .quantity = 100, .price = 1000},
          OrderExecuted{.order_id = 1, .quantity = 30});

  EXPECT_THAT(Depth(book().bids()), ElementsAre(Pair(1000, 70)));
}

TEST_F(FeedHandlerTest, FullExecutionRemovesOrderFromBookAndIndex) {
  Process(AddOrder{.order_id = 1, .quantity = 100, .price = 1000},
          OrderExecuted{.order_id = 1, .quantity = 100},
          // Must not find the order any more.
          OrderDelete{.order_id = 1});

  EXPECT_THAT(book().bids(), IsEmpty());
}

TEST_F(FeedHandlerTest, ExecutionWithPriceAndCancelReduceQuantity) {
  Process(AddOrder{.order_id = 1, .quantity = 100, .price = 1000},
          OrderExecutedWithPrice{.order_id = 1, .quantity = 20, .price = 990},
          OrderCancel{.order_id = 1, .quantity = 30});

  EXPECT_THAT(Depth(book().bids()), ElementsAre(Pair(1000, 50)));
}

TEST_F(FeedHandlerTest, CancelOfRemainingQuantityRemovesOrder) {
  Process(AddOrder{.order_id = 1, .quantity = 100, .price = 1000},
          OrderCancel{.order_id = 1, .quantity = 100});

  EXPECT_THAT(book().bids(), IsEmpty());
}

TEST_F(FeedHandlerTest, DeleteRemovesOnlyThatOrder) {
  Process(AddOrder{.order_id = 1, .quantity = 10, .price = 1000},
          AddOrder{.order_id = 2, .quantity = 20, .price = 1000},
          AddOrder{.order_id = 3, .quantity = 30, .price = 1000},
          OrderDelete{.order_id = 2});

  EXPECT_THAT(QueueIds(book().bids().at(1000)), ElementsAre(1, 3));
}

TEST_F(FeedHandlerTest, ReplaceMovesOrderAndKeepsSide) {
  Process(AddOrder{.order_id = 1, .side = 'S', .quantity = 100, .price = 1000},
          OrderReplace{.original_order_id = 1,
                       .new_order_id = 2,
                       .quantity = 50,
                       .price = 1005});

  EXPECT_THAT(book().bids(), IsEmpty());
  EXPECT_THAT(Depth(book().asks()), ElementsAre(Pair(1005, 50)));
  EXPECT_THAT(QueueIds(book().asks().at(1005)), ElementsAre(2));
}

TEST_F(FeedHandlerTest, ReplaceLosesTimePriority) {
  Process(AddOrder{.order_id = 1, .quantity = 10, .price = 1000},
          AddOrder{.order_id = 2, .quantity = 20, .price = 1000},
          OrderReplace{.original_order_id = 1,
                       .new_order_id = 3,
                       .quantity = 10,
                       .price = 1000});

  EXPECT_THAT(QueueIds(book().bids().at(1000)), ElementsAre(2, 3));
}

TEST_F(FeedHandlerTest, ReplacedOrderIsTrackedUnderNewId) {
  Process(AddOrder{.order_id = 1, .quantity = 10, .price = 1000},
          OrderReplace{.original_order_id = 1,
                       .new_order_id = 2,
                       .quantity = 10,
                       .price = 1000},
          OrderDelete{.order_id = 1});
  EXPECT_THAT(Depth(book().bids()), ElementsAre(Pair(1000, 10)));

  Process(OrderDelete{.order_id = 2});
  EXPECT_THAT(book().bids(), IsEmpty());
}

TEST_F(FeedHandlerTest, IgnoresUnknownOrderIds) {
  Process(AddOrder{.order_id = 1, .quantity = 100, .price = 1000},
          OrderExecuted{.order_id = 99, .quantity = 10},
          OrderCancel{.order_id = 99, .quantity = 10},
          OrderReplace{.original_order_id = 99,
                       .new_order_id = 100,
                       .quantity = 10,
                       .price = 1000},
          OrderDelete{.order_id = 99});

  EXPECT_THAT(Depth(book().bids()), ElementsAre(Pair(1000, 100)));
}

TEST_F(FeedHandlerTest, SkipsUnhandledMessageTypes) {
  std::vector<uint8_t> feed =
      Feed(AddOrder{.order_id = 1, .quantity = 10, .price = 1000});
  std::vector<uint8_t> system_event(12, 0);
  system_event[0] = 'S';
  AppendFramed(feed, system_event);
  AppendFramed(feed, itch_test::Encode(AddOrder{
                         .order_id = 2, .quantity = 20, .price = 1000}));

  handler_.Process(feed);

  EXPECT_THAT(QueueIds(book().bids().at(1000)), ElementsAre(1, 2));
}

TEST_F(FeedHandlerTest, StopsAtTruncatedMessage) {
  std::vector<uint8_t> feed =
      Feed(AddOrder{.order_id = 1, .quantity = 10, .price = 1000});
  std::vector<uint8_t> second =
      itch_test::Encode(AddOrder{.order_id = 2, .quantity = 20, .price = 1000});
  AppendFramed(feed, second);
  feed.resize(feed.size() - 1);  // cut the last byte of the second message

  handler_.Process(feed);

  EXPECT_THAT(QueueIds(book().bids().at(1000)), ElementsAre(1));
}

TEST_F(FeedHandlerTest, StopsAtZeroLengthMessage) {
  std::vector<uint8_t> feed =
      Feed(AddOrder{.order_id = 1, .quantity = 10, .price = 1000});
  AppendFramed(feed, {});
  AppendFramed(feed, itch_test::Encode(AddOrder{
                         .order_id = 2, .quantity = 20, .price = 1000}));

  handler_.Process(feed);

  EXPECT_THAT(QueueIds(book().bids().at(1000)), ElementsAre(1));
}

// Invariant checks abort in Debug builds; in Release the statements just run.

TEST(FeedHandlerDeathTest, DuplicateOrderIdAborts) {
  FeedHandler handler;
  handler.Process(Feed(AddOrder{.order_id = 1, .quantity = 10, .price = 1000}));
  const auto feed = Feed(AddOrder{.order_id = 1, .quantity = 10, .price = 999});
  EXPECT_DEBUG_DEATH(handler.Process(feed), "duplicate order_id=1");
}

TEST(FeedHandlerDeathTest, MessageForOtherStockAborts) {
  FeedHandler handler;
  handler.Process(Feed(
      AddOrder{.stock_locate = 1, .order_id = 1, .quantity = 10, .price = 1}));
  const auto feed = Feed(OrderDelete{.stock_locate = 2, .order_id = 1});
  EXPECT_DEBUG_DEATH(handler.Process(feed), "indexed under 1");
}

TEST(FeedHandlerDeathTest, StockDirectoryOverNonEmptyBookAborts) {
  FeedHandler handler;
  handler.Process(Feed(AddOrder{.order_id = 1, .quantity = 10, .price = 1}));
  const auto feed = Feed(StockDirectory{.symbol = "AAPL"});
  EXPECT_DEBUG_DEATH(handler.Process(feed), "stock_locate=1 name=AAPL");
}

TEST(FeedHandlerDeathTest, ReplaceThatCrossesBookAborts) {
  FeedHandler handler;
  handler.Process(Feed(
      AddOrder{.order_id = 1, .side = 'B', .quantity = 10, .price = 1000},
      AddOrder{.order_id = 2, .side = 'S', .quantity = 10, .price = 1005}));
  const auto feed = Feed(OrderReplace{
      .original_order_id = 2, .new_order_id = 3, .quantity = 10, .price = 999});
  EXPECT_DEBUG_DEATH(handler.Process(feed), "best_bid=1000 best_ask=999");
}

TEST(FeedHandlerDeathTest, CancelBeyondRemainingQuantityAborts) {
  FeedHandler handler;
  handler.Process(Feed(AddOrder{.order_id = 1, .quantity = 10, .price = 1000}));
  const auto feed = Feed(OrderCancel{.order_id = 1, .quantity = 11});
  EXPECT_DEBUG_DEATH(handler.Process(feed), "remaining=10 reduce_by=11");
}

}  // namespace
