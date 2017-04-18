#pragma once

#include <chrono>

#include <core/distributed.hh>
#include <core/reactor.hh>

#include "rpc.hh"

namespace seaperf {
namespace server {

class BenchmarkService : public seastar::sharded<BenchmarkService> {
  public:
    void listen(ipv4_addr addr);
    future<> stop();
    bool is_stopping() { return m_stopping; }

  private:
    using rpc_protocol = rpc::protocol<serializer>;
    std::unique_ptr<rpc_protocol> m_protocol;
    std::unique_ptr<rpc_protocol::server> m_server;

    bool m_stopping {false};
};

class TCPBenchmarkConn {
 public:
  future<BenchmarkResult> listen_and_run(ipv4_addr addr);
  void set_bench_duration(timer<>::duration t);

 private:
  future<> run();

  server_socket m_listener;
  connected_socket m_data_sock;
  input_stream<char> m_in;
  output_stream<char> m_out;

  timer<> m_bench_timer;
  timer<>::duration m_bench_duration;
  bool m_is_time_up = false;
  int64_t m_byte_cnt = 0;

  BenchmarkResult m_result;
};

}  // namespace server
}  // namespace seaperf
