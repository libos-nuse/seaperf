#pragma once

#include <chrono>

#include <core/distributed.hh>
#include <core/reactor.hh>

namespace seaperf {
namespace server {

class Conn {
 public:
  Conn(connected_socket&& sock, socket_address) : m_sock{std::move(sock)} {
    m_in = m_sock.input();
    m_out = m_sock.output();
  }

  future<> process();
  void set_bench_duration(timer<>::duration t);

 private:
  connected_socket m_sock;
  input_stream<char> m_in;
  output_stream<char> m_out;

  timer<>::duration m_bench_duration;
  bool m_is_time_up = false;
  int64_t m_byte_cnt = 0;
  timer<> m_bench_timer;
};

class Server : public seastar::sharded<Server> {
 public:
  Server() {}

  future<> listen(ipv4_addr addr, uint64_t duration_sec);
  future<> stop();
  future<> stopped();

 private:
  future<> do_accepts();

  server_socket m_listener;
  uint64_t m_duration_sec;
  promise<> m_all_connections_stopped;
  future<> m_stopped = m_all_connections_stopped.get_future();
};
}
}
