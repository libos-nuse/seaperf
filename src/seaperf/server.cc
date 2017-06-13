#include <core/byteorder.hh>
#include <core/reactor.hh>
#include "benchmark.hh"
#include "server.hh"

namespace seaperf {
namespace server {

future<> Server::listen(ipv4_addr addr, bool once) {
  listen_options lo;
  lo.reuse_address = true;
  m_listener = engine().listen(make_ipv4_address(addr), lo);
  m_once = once;
  do_accepts();
  return make_ready_future<>();
}

future<> Server::do_accepts() {
  return m_listener.accept()
      .then_wrapped(
          [this](future<connected_socket, socket_address> f_cs_sa) mutable {
            if (m_stopping) {
              maybe_idle();
              return;
            }

            auto cs_sa = f_cs_sa.get();
            auto conn = new BenchmarkConn{*this, std::get<0>(std::move(cs_sa)),
                                          std::get<1>(std::move(cs_sa))};
            conn->process().then_wrapped([this, conn](auto&& f) {
              delete conn;
              try {
                f.get();
              } catch (std::exception& ex) {
                std::cerr << "request error: " << ex.what() << std::endl;
              }
              if (m_once) {
                engine().exit(0);
              }
            });

            do_accepts();
          })
      .then_wrapped([](auto f) {
        try {
          f.get();
        } catch (std::exception& ex) {
          std::cerr << "accept failed: " << ex.what() << std::endl;
        }
      });
}

void Server::register_connection() { m_active_connections++; }

void Server::unregister_connection() {
  m_active_connections--;
  maybe_idle();
}

void Server::maybe_idle() {
  if (m_stopping && !m_active_connections) {
    m_all_connections_stopped.set_value();
  }
}

future<> Server::stop() {
  m_stopping = true;
  m_listener.abort_accept();
  return std::move(m_stopped);
}

future<> Server::stopped() { return m_stopped.discard_result(); }

BenchmarkConn::BenchmarkConn(Server& s, connected_socket&& sock, socket_address)
    : server{s}, m_sock{std::move(sock)} {
  m_in = m_sock.input();
  m_out = m_sock.output();
  server.register_connection();
}

BenchmarkConn::~BenchmarkConn() { server.unregister_connection(); }

future<> BenchmarkConn::process() {
  m_is_time_up = false;
  m_bench_timer.set_callback([this] { m_is_time_up = true; });

  return m_in.read_exactly(sizeof(BenchmarkRequest))
      .then([this](auto buf) mutable {
        BenchmarkRequest req =
            *reinterpret_cast<const BenchmarkRequest*>(buf.get());
        m_packet_size = le_to_cpu(req.packet_size);
        req.duration = le_to_cpu(req.duration);
        m_bench_duration = std::chrono::seconds{req.duration};
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
        });
      })
      .then([this]() mutable {
        BenchmarkResult res;
        res.byte_cnt = cpu_to_le(m_byte_cnt);
        return m_out.write(reinterpret_cast<char*>(&res), sizeof(res));
      })
      .then([this]() mutable {
        auto bench_sec = m_bench_duration.count();
        auto mbit_cnt = 8 * m_byte_cnt / 1000000;
        std::cout << "duration sec, bytes sent, packet size, throughput, "
                     "throuput unit\n"
                  << bench_sec << ',' << m_byte_cnt << ',' << m_packet_size
                  << ',' << mbit_cnt / bench_sec << ',' << "10^6bits\n";

        return when_all(m_in.close(), m_out.close()).discard_result();
      });
}
}  // namespace server
}  // namespace seaperf
