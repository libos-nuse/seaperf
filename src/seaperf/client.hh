#pragma once

#include <core/distributed.hh>
#include <core/reactor.hh>

namespace seaperf {
namespace client {

std::unique_ptr<rpc::protocol<serializer>::client> connect(ipv4_addr addr);

class Client {
 public:
  Client();

  future<> run(ipv4_addr addr);
  void set_bench_duration(std::chrono::seconds t);
  void set_bufsize(size_t bufsize);

 private:
  future<> benchmark();

  connected_socket m_sock;
  input_stream<char> m_in;
  output_stream<char> m_out;

  std::string m_sendbuf;
  size_t m_sendbuf_size{6400};
  std::chrono::seconds m_bench_duration;
  bool m_is_time_up = false;
  timer<> m_bench_timer;
  std::chrono::seconds m_margin{5};
};
}
}
