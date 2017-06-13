#pragma once

#include <atomic>
#include <chrono>

#include <core/distributed.hh>
#include <core/reactor.hh>

namespace seaperf {
namespace server {

class Server : public seastar::sharded<Server> {
 public:
  Server() {}

  future<> listen(ipv4_addr addr, bool once = false);
  future<> stop();
  future<> stopped();

  void register_connection();
  void unregister_connection();

 private:
  future<> do_accepts();
  void maybe_idle();

  server_socket m_listener;
  bool m_once = false;
  bool m_stopping = false;
  std::atomic<int> m_active_connections{0};
  promise<> m_all_connections_stopped;
  future<> m_stopped = m_all_connections_stopped.get_future();
};

class BenchmarkConn {
 public:
  BenchmarkConn(Server& s, connected_socket&& sock, socket_address);
  ~BenchmarkConn();
  future<> process();

 private:
  Server& server;
  connected_socket m_sock;
  input_stream<char> m_in;
  output_stream<char> m_out;

  bool m_is_time_up = false;
  uint64_t m_byte_cnt = 0;
  std::chrono::seconds m_bench_duration;
  timer<> m_bench_timer;

  uint64_t m_packet_size;
};
}
}
