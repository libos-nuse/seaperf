#include <cstdint>
#include "core/reactor.hh"

namespace seaperf {

using BenchmarkRequest = struct  {
  net::packed<uint64_t> duration;
} __attribute__((packed));

using BenchmarkResult = struct  {
  net::packed<uint64_t> byte_cnt;
} __attribute__((packed));
}  // namespace seaperf
