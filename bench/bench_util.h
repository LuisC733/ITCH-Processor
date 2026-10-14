#ifndef ITCH_PROCESSOR_BENCH_BENCH_UTIL_H_
#define ITCH_PROCESSOR_BENCH_BENCH_UTIL_H_
#include <cstddef>
#include <cstdint>

#include <benchmark/benchmark.h>

// Reports throughput (items_per_second) and time_per_msg for benchmarks that
// process a fixed number of messages per iteration.
inline void SetMessageCounters(benchmark::State& state,
                               const std::size_t messages_per_iteration) {
  state.SetItemsProcessed(
      static_cast<int64_t>(state.iterations() * messages_per_iteration));
  state.counters["time_per_msg"] =
      benchmark::Counter(static_cast<double>(messages_per_iteration),
                         benchmark::Counter::kIsIterationInvariantRate |
                             benchmark::Counter::kInvert);
}

#endif  // ITCH_PROCESSOR_BENCH_BENCH_UTIL_H_
