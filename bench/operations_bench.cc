// Steady-state cost of single operations on one book with range(0) resting
// orders, measured through FeedHandler::Process (parse, order index, book).
// Each iteration replays a batch of messages that leaves the book's levels as
// it found them, so iterations are comparable; batches rotate so that large
// books do not stay in cache.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#include "bench_util.h"
#include "common/types.h"
#include "feed_handler/feed_handler.h"
#include "itch_builder.h"

namespace {

using ::itch_test::AddOrder;
using ::itch_test::AppendFramed;
using ::itch_test::Encode;
using ::itch_test::OrderDelete;
using ::itch_test::OrderExecuted;
using ::itch_test::OrderReplace;

constexpr int kLevels = 100;  // per side
constexpr Price kBestBid = 100'000;
constexpr Price kBestAsk = kBestBid + 1;
constexpr std::size_t kBatch = 1'000;
constexpr OrderId kBatchIdBase = 1'000'000'000;
// Large enough that partial executions never fill a resting order.
constexpr Quantity kRestingQuantity = 1'000'000'000;

struct Resting {
  OrderId order_id;
  char side;
  Price price;
};

Price LevelPrice(const char side, const int level) {
  return side == 'B' ? kBestBid - level : kBestAsk + level;
}

int RandomLevel(std::mt19937_64& rng) {
  return static_cast<int>(rng() % kLevels);
}

// A handler holding `count` resting orders spread evenly over kLevels bid and
// kLevels ask levels of stock locate 1.
class Book {
 public:
  explicit Book(const std::size_t count)
      : handler_(std::make_unique<FeedHandler>()) {
    std::vector<uint8_t> feed;
    for (std::size_t i = 0; i < count; ++i) {
      const char side = i % 2 == 0 ? 'B' : 'S';
      const Resting& order = resting_.emplace_back(Resting{
          i + 1, side, LevelPrice(side, static_cast<int>(i / 2 % kLevels))});
      AppendFramed(feed, Encode(AddOrder{.order_id = order.order_id,
                                         .side = order.side,
                                         .quantity = kRestingQuantity,
                                         .price = order.price}));
    }
    handler_->Process(feed);
  }

  FeedHandler& handler() { return *handler_; }
  const Resting& Random(std::mt19937_64& rng) const {
    return resting_[rng() % resting_.size()];
  }

 private:
  std::unique_ptr<FeedHandler> handler_;
  std::vector<Resting> resting_;
};

// Enough distinct batches that one cycle touches about every resting order,
// so large books are not measured with a warm cache.
std::size_t BatchCount(const std::size_t resting) {
  return std::max<std::size_t>(1, resting / kBatch);
}

// Builds BatchCount() batches with make_batch(book, rng, first_id), where
// first_id starts a range of kBatch ids unused by other batches, then replays
// them round-robin, one batch per iteration.
template <typename MakeBatch>
void Run(benchmark::State& state, const std::size_t messages_per_batch,
         MakeBatch make_batch) {
  Book book(state.range(0));
  std::mt19937_64 rng(42);
  std::vector<std::vector<uint8_t>> batches;
  for (std::size_t i = 0; i < BatchCount(state.range(0)); ++i) {
    batches.push_back(make_batch(book, rng, kBatchIdBase + i * kBatch));
  }
  std::size_t next = 0;
  for (auto _ : state) {
    book.handler().Process(batches[next]);
    next = next + 1 == batches.size() ? 0 : next + 1;
  }
  SetMessageCounters(state, messages_per_batch);
}

// Deletes ids in random order.
void AppendDeletes(std::vector<uint8_t>& batch, std::vector<OrderId> ids,
                   std::mt19937_64& rng) {
  std::shuffle(ids.begin(), ids.end(), rng);
  for (const OrderId id : ids) {
    AppendFramed(batch, Encode(OrderDelete{.order_id = id}));
  }
}

// kBatch adds onto existing levels, then deletes them in random order.
void BM_AddDeleteExistingLevel(benchmark::State& state) {
  Run(state, 2 * kBatch,
      [](const Book&, std::mt19937_64& rng, const OrderId first_id) {
        std::vector<uint8_t> batch;
        std::vector<OrderId> ids;
        for (std::size_t i = 0; i < kBatch; ++i) {
          const char side = rng() % 2 == 0 ? 'B' : 'S';
          ids.push_back(first_id + i);
          AppendFramed(
              batch,
              Encode(AddOrder{.order_id = ids.back(),
                              .side = side,
                              .quantity = 100,
                              .price = LevelPrice(side, RandomLevel(rng))}));
        }
        AppendDeletes(batch, ids, rng);
        return batch;
      });
}

// Like above, but every add opens a new price level behind the existing ones
// and every delete removes it again.
void BM_AddDeleteNewLevel(benchmark::State& state) {
  Run(state, 2 * kBatch,
      [](const Book&, std::mt19937_64& rng, const OrderId first_id) {
        std::vector<uint8_t> batch;
        std::vector<OrderId> ids;
        for (std::size_t i = 0; i < kBatch; ++i) {
          const char side = i % 2 == 0 ? 'B' : 'S';
          const int level = kLevels + static_cast<int>(i / 2);
          ids.push_back(first_id + i);
          AppendFramed(batch,
                       Encode(AddOrder{.order_id = ids.back(),
                                       .side = side,
                                       .quantity = 100,
                                       .price = LevelPrice(side, level)}));
        }
        AppendDeletes(batch, ids, rng);
        return batch;
      });
}

// kBatch partial executions of one share on random resting orders.
void BM_ExecutePartial(benchmark::State& state) {
  Run(state, kBatch, [](const Book& book, std::mt19937_64& rng, OrderId) {
    std::vector<uint8_t> batch;
    for (std::size_t i = 0; i < kBatch; ++i) {
      AppendFramed(batch,
                   Encode(OrderExecuted{.order_id = book.Random(rng).order_id,
                                        .quantity = 1}));
    }
    return batch;
  });
}

// kBatch / 2 resting orders replaced onto another level of the same side and
// straight back under their original id and price.
void BM_Replace(benchmark::State& state) {
  Run(state, kBatch,
      [](const Book& book, std::mt19937_64& rng, const OrderId first_id) {
        std::vector<uint8_t> batch;
        for (std::size_t i = 0; i < kBatch / 2; ++i) {
          const Resting& order = book.Random(rng);
          const OrderId temporary_id = first_id + i;
          AppendFramed(batch,
                       Encode(OrderReplace{
                           .original_order_id = order.order_id,
                           .new_order_id = temporary_id,
                           .quantity = kRestingQuantity,
                           .price = LevelPrice(order.side, RandomLevel(rng))}));
          AppendFramed(batch,
                       Encode(OrderReplace{.original_order_id = temporary_id,
                                           .new_order_id = order.order_id,
                                           .quantity = kRestingQuantity,
                                           .price = order.price}));
        }
        return batch;
      });
}

// Resting orders in the book: small, about the BX day's peak across all books
// (~57k), large.
void BookSizes(benchmark::internal::Benchmark* benchmark) {
  benchmark->Arg(1 << 10)->Arg(1 << 16)->Arg(1 << 20);
}

BENCHMARK(BM_AddDeleteExistingLevel)->Apply(BookSizes);
BENCHMARK(BM_AddDeleteNewLevel)->Apply(BookSizes);
BENCHMARK(BM_ExecutePartial)->Apply(BookSizes);
BENCHMARK(BM_Replace)->Apply(BookSizes);

}  // namespace
