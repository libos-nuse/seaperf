#include <core/reactor.hh>
#include "rpc.hh"
#include "server.hh"

namespace seaperf {
namespace server {

std::unique_ptr<rpc::protocol<serializer>::server> listen(ipv4_addr addr) {
  rpc::protocol<serializer> protocol{{}};

  protocol.register_handler(RPC_TCPBENCH, [=](uint64_t duration_sec) {
    auto conn = std::make_unique<BenchmarkConn>();
    auto duration = std::chrono::seconds{duration_sec};
    conn->set_bench_duration(duration);
    return do_with(std::move(conn),
                   [addr](auto& conn) { return conn->listen_and_run(addr); });
  });

  return std::make_unique<rpc::protocol<serializer>::server>(protocol, addr);
}

future<BenchmarkResult> BenchmarkConn::listen_and_run(ipv4_addr addr) {
  listen_options lo;
  lo.reuse_address = true;
  m_listener = engine().listen(make_ipv4_address(addr), lo);
  return m_listener.accept()
      .then([this](auto data_sock, auto) mutable {
        m_data_sock = std::move(data_sock);
        m_in = std::move(data_sock.input());
        m_out = std::move(data_sock.output());
        return this->run();
      })
      .then([this]() mutable {
        m_listener.abort_accept();
        return when_all(m_in.close(), m_out.close()).discard_result();
      })
      .then([this] { return make_ready_future<BenchmarkResult>(m_result); });
}

void BenchmarkConn::set_bench_duration(timer<>::duration t) {
  m_bench_duration = t;
}

future<> BenchmarkConn::run() {
  m_is_time_up = false;
  m_bench_timer.set_callback([this] { m_is_time_up = true; });
  m_bench_timer.arm(m_bench_duration);

  return repeat([this]() mutable {
           return m_in.read().then([this](auto buf) mutable {
             if (m_is_time_up) {
               return make_ready_future<stop_iteration>(stop_iteration::yes);
             }
             if (buf) {
               m_byte_cnt += buf.size();
               return make_ready_future<stop_iteration>(stop_iteration::no);
             } else {
               return make_ready_future<stop_iteration>(stop_iteration::yes);
             }
           });
         })
      .then([this]() mutable {
        using namespace std::chrono;
        auto bench_sec = duration_cast<seconds>(m_bench_duration).count();
        m_result.duration_sec = bench_sec;
        m_result.byte_cnt = m_byte_cnt;
        return make_ready_future<>();
      });
}
}  // namespace server
}  // namespace seaperf
