#include <cstdint>
#include "core/reactor.hh"

namespace seaperf {

using BenchmarkRequest = struct {
  net::packed<uint64_t> duration;
};

using BenchmarkResult = struct {
  net::packed<uint64_t> byte_cnt;
};
}  // namespace seaperf
