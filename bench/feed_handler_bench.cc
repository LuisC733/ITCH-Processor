// End-to-end replay of a full ITCH day (ITCH_DATA_FILE in CMake). Handler
// construction and destruction are not timed, loading the file happens once.

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include <benchmark/benchmark.h>

#include "bench_util.h"
#include "feed_handler/feed_handler.h"
#include "parser/parser.h"
#include "reader/reader.h"

namespace {

struct Day {
  std::vector<uint8_t> data;
  std::size_t messages = 0;
};

const Day& LoadDay() {
  static const Day day = [] {
    Day day;
    if (!std::filesystem::exists(ITCH_DATA_FILE)) {
      return day;
    }
    day.data = ReadFile(ITCH_DATA_FILE);
    for (std::size_t offset = 0; offset + 2 <= day.data.size();
         ++day.messages) {
      offset += 2 + ReadBigEndian<uint16_t>(day.data.data() + offset);
    }
    return day;
  }();
  return day;
}

void BM_ReplayDay(benchmark::State& state) {
  const Day& day = LoadDay();
  if (day.data.empty()) {
    state.SkipWithError("ITCH data file not found: " ITCH_DATA_FILE);
    return;
  }
  for (auto _ : state) {
    state.PauseTiming();
    auto handler = std::make_unique<FeedHandler>();
    state.ResumeTiming();

    handler->Process(day.data);

    state.PauseTiming();
    handler.reset();
    state.ResumeTiming();
  }
  SetMessageCounters(state, day.messages);
}
BENCHMARK(BM_ReplayDay)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(1)
    ->Repetitions(5)
    ->DisplayAggregatesOnly(true);

}  // namespace
