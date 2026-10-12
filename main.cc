#include <cstdint>
#include <vector>

#include "feed_handler/feed_handler.h"
#include "reader/reader.h"

int main() {
  const std::vector<uint8_t> src = ReadFile("assets/20200130.BX_ITCH_50");
  FeedHandler handler;
  handler.Process(src);
  return 0;
}
