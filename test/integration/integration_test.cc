// Replays a full ITCH day and compares the books at fixed times of day against
// snapshots from reference.py, an independent reconstruction in Python. In
// Debug builds every invariant check runs over the whole day as well.
//
// Skipped when the data file (ITCH_DATA_FILE in CMake) is missing.

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <ostream>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/types.h"
#include "feed_handler/feed_handler.h"
#include "orderbook/order.h"
#include "orderbook/orderbook.h"
#include "parser/parser.h"
#include "reader/reader.h"

namespace {

using ::testing::ElementsAreArray;

const std::filesystem::path kDataFile = ITCH_DATA_FILE;
const std::filesystem::path kReferenceFile = ITCH_REFERENCE_FILE;

// Must match DETAIL_LEVELS and the digest in reference.py.
constexpr std::size_t kDetailLevels = 5;
constexpr uint64_t kDigestSeed = 0xCBF29CE484222325;
constexpr uint64_t kDigestPrime = 0x100000001B3;

struct Level {
  Price price = 0;
  uint64_t quantity = 0;
  std::size_t orders = 0;

  bool operator==(const Level&) const = default;
};

std::ostream& operator<<(std::ostream& os, const Level& level) {
  return os << level.quantity << "@" << level.price << " (" << level.orders
            << " orders)";
}

struct BookTop {
  std::vector<Level> bids;
  std::vector<Level> asks;
};

struct Checkpoint {
  uint64_t time = 0;  // ns since midnight
  std::size_t live_orders = 0;
  std::size_t active_books = 0;
  uint64_t digest = 0;
  std::map<StockLocate, BookTop> tops;
};

struct Reference {
  std::vector<Checkpoint> checkpoints;
  std::map<StockLocate, std::string> detail_books;  // locate -> symbol
};

Reference LoadReference(const std::filesystem::path& path) {
  Reference reference;
  std::ifstream in(path);
  std::string line;
  while (std::getline(in, line)) {
    std::istringstream fields(line);
    std::string kind;
    fields >> kind;
    if (kind == "checkpoint") {
      Checkpoint& checkpoint = reference.checkpoints.emplace_back();
      fields >> checkpoint.time >> checkpoint.live_orders >>
          checkpoint.active_books >> std::hex >> checkpoint.digest;
    } else if (kind == "level") {
      uint64_t time = 0;
      StockLocate stock_locate = 0;
      std::string symbol;
      char side = 0;
      std::size_t rank = 0;
      Level level;
      fields >> time >> stock_locate >> symbol >> side >> rank >> level.price >>
          level.quantity >> level.orders;
      BookTop& top = reference.checkpoints.back().tops[stock_locate];
      (side == 'B' ? top.bids : top.asks).push_back(level);
      reference.detail_books[stock_locate] = symbol;
    }
  }
  return reference;
}

uint64_t Timestamp(const uint8_t* msg) {
  return uint64_t{ReadBigEndian<uint16_t>(msg + 5)} << 32 |
         ReadBigEndian<uint32_t>(msg + 7);
}

// Offset of the first message at or after offset with a timestamp >= time.
std::size_t FindCheckpoint(const std::span<const uint8_t> data,
                           std::size_t offset, const uint64_t time) {
  while (offset + 2 <= data.size()) {
    const uint16_t length = ReadBigEndian<uint16_t>(data.data() + offset);
    if (length == 0 || offset + 2 + length > data.size() ||
        Timestamp(data.data() + offset + 2) >= time) {
      return offset;
    }
    offset += 2 + length;
  }
  return data.size();
}

template <typename T>
std::vector<Level> Levels(const T& map, const std::size_t limit) {
  std::vector<Level> levels;
  for (const auto& [price, price_level] : map) {
    if (levels.size() == limit) {
      break;
    }
    Level& level = levels.emplace_back(Level{price, 0, price_level.size()});
    for (const Order& order : price_level) {
      level.quantity += order.quantity;
    }
  }
  return levels;
}

class Digest {
 public:
  void Add(const uint64_t word) { hash_ = (hash_ ^ word) * kDigestPrime; }
  void Add(const std::vector<Level>& levels) {
    Add(levels.size());
    for (const Level& level : levels) {
      Add(level.price);
      Add(level.quantity);
      Add(level.orders);
    }
  }
  uint64_t value() const { return hash_; }

 private:
  uint64_t hash_ = kDigestSeed;
};

std::string FormatTime(const uint64_t time) {
  constexpr uint64_t kNsPerMinute = 60'000'000'000;
  if (time == UINT64_MAX) {
    return "end of file";
  }
  std::ostringstream out;
  out << time / (60 * kNsPerMinute) << ":" << (time / kNsPerMinute) % 60 / 10
      << (time / kNsPerMinute) % 10;
  return out.str();
}

void ExpectMatches(const FeedHandler& handler, const Checkpoint& expected,
                   const Reference& reference) {
  std::size_t live_orders = 0;
  std::size_t active_books = 0;
  Digest digest;
  for (std::size_t locate = 0; locate <= UINT16_MAX; ++locate) {
    const Orderbook& book = handler.book(static_cast<StockLocate>(locate));
    if (book.bids().empty() && book.asks().empty()) {
      continue;
    }
    const std::vector<Level> bids = Levels(book.bids(), SIZE_MAX);
    const std::vector<Level> asks = Levels(book.asks(), SIZE_MAX);
    for (const Level& level : bids) {
      live_orders += level.orders;
    }
    for (const Level& level : asks) {
      live_orders += level.orders;
    }
    ++active_books;
    digest.Add(locate);
    digest.Add(bids);
    digest.Add(asks);
  }
  EXPECT_EQ(live_orders, expected.live_orders);
  EXPECT_EQ(active_books, expected.active_books);
  EXPECT_EQ(digest.value(), expected.digest)
      << "full-depth digest differs, see reference.py for the encoding";

  for (const auto& [locate, symbol] : reference.detail_books) {
    SCOPED_TRACE(symbol);
    const Orderbook& book = handler.book(locate);
    const auto top = expected.tops.find(locate);
    const BookTop empty;
    const BookTop& expected_top =
        top == expected.tops.end() ? empty : top->second;
    EXPECT_EQ(book.name(), symbol);
    EXPECT_THAT(Levels(book.bids(), kDetailLevels),
                ElementsAreArray(expected_top.bids));
    EXPECT_THAT(Levels(book.asks(), kDetailLevels),
                ElementsAreArray(expected_top.asks));
  }
}

TEST(IntegrationTest, BooksMatchReferenceThroughoutTheDay) {
  if (!std::filesystem::exists(kDataFile)) {
    GTEST_SKIP() << "ITCH data file not found: " << kDataFile;
  }
  const Reference reference = LoadReference(kReferenceFile);
  ASSERT_FALSE(reference.checkpoints.empty())
      << "no reference at " << kReferenceFile;
  const std::vector<uint8_t> data = ReadFile(kDataFile.c_str());

  FeedHandler handler;
  std::size_t offset = 0;
  for (const Checkpoint& checkpoint : reference.checkpoints) {
    const std::size_t end = FindCheckpoint(data, offset, checkpoint.time);
    handler.Process(std::span(data).subspan(offset, end - offset));
    offset = end;

    SCOPED_TRACE("at " + FormatTime(checkpoint.time));
    ExpectMatches(handler, checkpoint, reference);
  }
  EXPECT_EQ(offset, data.size()) << "trailing bytes after the last message";
}

}  // namespace
