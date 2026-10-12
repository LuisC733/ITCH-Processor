#include "parser/parser.h"

#include <cstdint>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/side.h"
#include "itch_builder.h"

namespace {

using ::testing::ElementsAreArray;

// Messages written by hand from the ITCH 5.0 spec, independent of the builder,
// with non-zero tracking number and timestamp to make sure they are skipped.
const std::vector<uint8_t> kAddOrder = {
    'A',                                             // message type
    0x00, 0x07,                                      // stock locate = 7
    0x00, 0x02,                                      // tracking number
    0x00, 0x00, 0x0B, 0x2D, 0x3E, 0x4F,              // timestamp
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x57,  // order ref = 1111
    'S',                                             // side
    0x00, 0x00, 0x00, 0x64,                          // shares = 100
    'A',  'A',  'P',  'L',  ' ',  ' ',  ' ',  ' ',   // stock
    0x00, 0x16, 0xE3, 0x60,                          // price = 150.0000
};

const std::vector<uint8_t> kOrderExecuted = {
    'E',                                             // message type
    0x00, 0x07,                                      // stock locate = 7
    0x00, 0x02,                                      // tracking number
    0x00, 0x00, 0x0B, 0x2D, 0x3E, 0x4F,              // timestamp
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x57,  // order ref = 1111
    0x00, 0x00, 0x00, 0x32,                          // executed shares = 50
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09,  // match number
};

const std::vector<uint8_t> kOrderReplace = {
    'U',                                             // message type
    0x00, 0x07,                                      // stock locate = 7
    0x00, 0x02,                                      // tracking number
    0x00, 0x00, 0x0B, 0x2D, 0x3E, 0x4F,              // timestamp
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x57,  // original ref = 1111
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x58,  // new ref = 1112
    0x00, 0x00, 0x01, 0x2C,                          // shares = 300
    0x00, 0x16, 0xE3, 0xC4,                          // price = 150.0100
};

std::vector<uint8_t> StockDirectoryBytes(const char (&symbol)[9]) {
  std::vector<uint8_t> msg(39, 0);
  msg[0] = 'R';
  msg[2] = 0x07;  // stock locate = 7
  for (int i = 0; i < 8; ++i) {
    msg[11 + i] = symbol[i];
  }
  return msg;
}

// Zeroes tracking number and timestamp, which the builder leaves empty.
std::vector<uint8_t> WithoutTimestamp(std::vector<uint8_t> msg) {
  for (int i = 3; i < 11; ++i) {
    msg[i] = 0;
  }
  return msg;
}

TEST(ReadBigEndianTest, DecodesEachWidthFromUnalignedPointer) {
  const uint8_t bytes[] = {0xFF, 0x01, 0x02, 0x03, 0x04,
                           0x05, 0x06, 0x07, 0x08};
  EXPECT_EQ(ReadBigEndian<uint16_t>(bytes + 1), 0x0102u);
  EXPECT_EQ(ReadBigEndian<uint32_t>(bytes + 1), 0x01020304u);
  EXPECT_EQ(ReadBigEndian<uint64_t>(bytes + 1), 0x0102030405060708u);
}

TEST(ParserTest, ParsesAddOrder) {
  const Order order = ParseAddOrder(kAddOrder.data());
  EXPECT_EQ(order.stock_locate, 7);
  EXPECT_EQ(order.order_id, 1111u);
  EXPECT_EQ(order.side, Side::kSell);
  EXPECT_EQ(order.quantity, 100u);
  EXPECT_EQ(order.price, 1500000u);
}

TEST(ParserTest, ParsesBuySide) {
  std::vector<uint8_t> msg = kAddOrder;
  msg[19] = 'B';
  EXPECT_EQ(ParseAddOrder(msg.data()).side, Side::kBuy);
}

TEST(ParserTest, ParsesExecutionFields) {
  EXPECT_EQ(ParseStockLocate(kOrderExecuted.data()), 7);
  EXPECT_EQ(ParseOrderId(kOrderExecuted.data()), 1111u);
  EXPECT_EQ(ParseOrderQuantity(kOrderExecuted.data()), 50u);
}

TEST(ParserTest, ParsesReplaceOrder) {
  EXPECT_EQ(ParseOrderId(kOrderReplace.data()), 1111u);
  const Order order = ParseReplaceOrder(kOrderReplace.data());
  EXPECT_EQ(order.stock_locate, 7);
  EXPECT_EQ(order.order_id, 1112u);
  EXPECT_EQ(order.quantity, 300u);
  EXPECT_EQ(order.price, 1500100u);
}

TEST(ParserTest, StockDirectoryTrimsPadding) {
  const auto [stock_locate, name] =
      ParseStockDirectory(StockDirectoryBytes("AAPL    ").data());
  EXPECT_EQ(stock_locate, 7);
  EXPECT_EQ(name, "AAPL");
}

TEST(ParserTest, StockDirectoryKeepsFullWidthSymbol) {
  EXPECT_EQ(ParseStockDirectory(StockDirectoryBytes("ABCDEFGH").data()).second,
            "ABCDEFGH");
}

TEST(ItchBuilderTest, EncodesOrderExecutedPerSpec) {
  std::vector<uint8_t> expected = WithoutTimestamp(kOrderExecuted);
  expected[30] = 0;
  EXPECT_THAT(itch_test::Encode(itch_test::OrderExecuted{
                  .stock_locate = 7, .order_id = 1111, .quantity = 50}),
              ElementsAreArray(expected));
}

TEST(ItchBuilderTest, EncodesOrderReplacePerSpec) {
  EXPECT_THAT(
      itch_test::Encode(itch_test::OrderReplace{.stock_locate = 7,
                                                .original_order_id = 1111,
                                                .new_order_id = 1112,
                                                .quantity = 300,
                                                .price = 1500100}),
      ElementsAreArray(WithoutTimestamp(kOrderReplace)));
}

TEST(ItchBuilderTest, EncodesAddOrderPerSpec) {
  std::vector<uint8_t> expected = WithoutTimestamp(kAddOrder);
  for (int i = 0; i < 8; ++i) {
    expected[24 + i] = "TEST    "[i];
  }
  EXPECT_THAT(itch_test::Encode(itch_test::AddOrder{.stock_locate = 7,
                                                    .order_id = 1111,
                                                    .side = 'S',
                                                    .quantity = 100,
                                                    .price = 1500000}),
              ElementsAreArray(expected));
}

}  // namespace
