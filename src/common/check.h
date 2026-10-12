#ifndef ITCH_PROCESSOR_COMMON_CHECK_H_
#define ITCH_PROCESSOR_COMMON_CHECK_H_
#include <cstdlib>
#include <iostream>
#include <sstream>

// Debug-only invariant checks:
//
//   ITCH_DCHECK(quantity <= remaining) << "order_id=" << order_id;
//
// On failure, prints the location, the condition and the streamed context to
// stderr and aborts. With NDEBUG defined (Release builds) the check compiles to
// nothing and neither the condition nor the context is evaluated.

namespace check_internal {

class CheckFailure {
 public:
  CheckFailure(const char* file, int line, const char* condition) {
    stream_ << file << ":" << line << ": Invariant violated: " << condition
            << " ";
  }
  CheckFailure(const CheckFailure&) = delete;
  CheckFailure& operator=(const CheckFailure&) = delete;
  ~CheckFailure() {
    std::cerr << stream_.str() << std::endl;
    std::abort();
  }

  std::ostream& stream() { return stream_; }

 private:
  std::ostringstream stream_;
};

// Binds looser than << but tighter than ?:, so the whole streamed expression
// collapses to void and the macro can be used as a single statement.
struct Voidify {
  void operator&(std::ostream&) {}
};

}  // namespace check_internal

#define ITCH_DCHECK_IMPL(condition)                                        \
  (condition)                                                              \
      ? (void)0                                                            \
      : ::check_internal::Voidify() &                                      \
            ::check_internal::CheckFailure(__FILE__, __LINE__, #condition) \
                .stream()

#ifndef NDEBUG
#define ITCH_DCHECK(condition) ITCH_DCHECK_IMPL(condition)
#else
// Still type-checks the condition and context so they don't rot in Release.
#define ITCH_DCHECK(condition) \
  while (false) ITCH_DCHECK_IMPL(condition)
#endif

#endif  // ITCH_PROCESSOR_COMMON_CHECK_H_
