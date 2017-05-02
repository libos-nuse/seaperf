#pragma once

#include <chrono>

#include <core/distributed.hh>
#include <core/reactor.hh>

namespace seaperf {
namespace server {

class BenchmarkConn {
 public:
  BenchmarkConn(connected_socket&& sock, socket_address) : m_sock{std::move(sock)} {
    m_in = m_sock.input();
    m_out = m_sock.output();
  }

  future<> process();

 private:
  connected_socket m_sock;
  input_stream<char> m_in;
  output_stream<char> m_out;

  bool m_is_time_up = false;
  int64_t m_byte_cnt = 0;
  timer<> m_bench_timer;
};

class Server : public seastar::sharded<Server> {
 public:
  Server() {}

  future<> listen(ipv4_addr addr);
  future<> stop();
  future<> stopped();

 private:
  future<> do_accepts();

  server_socket m_listener;
  promise<> m_all_connections_stopped;
  future<> m_stopped = m_all_connections_stopped.get_future();
};
}
}
