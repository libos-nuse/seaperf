#include <cstdint>
#include "core/reactor.hh"

namespace seaperf {

using BenchmarkRequest = struct  {
  net::packed<uint64_t> duration;
  net::packed<uint64_t> packet_size;
} __attribute__((packed));

using BenchmarkResult = struct  {
  net::packed<uint64_t> byte_cnt;
} __attribute__((packed));
}  // namespace seaperf
