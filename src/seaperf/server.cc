#include <core/reactor.hh>
#include "server.hh"

namespace seaperf {
namespace server {

future<> Server::listen(ipv4_addr addr) {
  listen_options lo;
  lo.reuse_address = true;
  m_listener = engine().listen(make_ipv4_address(addr), lo);
  do_accepts();
  return make_ready_future<>();
}

future<> Server::do_accepts() {
  return m_listener.accept()
      .then_wrapped(
          [this](future<connected_socket, socket_address> f_cs_sa) mutable {
            auto cs_sa = f_cs_sa.get();
            auto conn = new Conn{std::get<0>(std::move(cs_sa)),
                                 std::get<1>(std::move(cs_sa))};
            // FIXME: hardcoded bench duration
            using namespace std::chrono_literals;
            conn->set_bench_duration(10s);
            conn->process().then_wrapped([this, conn](auto&& f) {
              delete conn;
              try {
                f.get();
              } catch (std::exception& ex) {
                std::cerr << "request error: " << ex.what() << std::endl;
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

future<> Server::stop() {
  // TODO
  return make_ready_future<>();
}

future<> Server::stopped() {
  return m_stopped.then([] { make_ready_future<>(); });
}

future<> Conn::process() {
  m_is_time_up = false;
  m_bench_timer.set_callback([this] { m_is_time_up = true; });
  m_bench_timer.arm(m_bench_duration);

  return repeat([this] {
           return m_in.read().then([this](auto buf) {
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
      .then([this] {
        using namespace std::chrono;
        auto bench_sec = duration_cast<seconds>(m_bench_duration).count();
        print("received: %d bytes\n", m_byte_cnt);
        print("duration: %d s\n", bench_sec);
        return when_all(m_in.close(), m_out.close()).then([](auto) {
          make_ready_future<>();
        });
      });
}

void Conn::set_bench_duration(timer<>::duration t) { m_bench_duration = t; }
}  // namespace server
}  // namespace seaperf
