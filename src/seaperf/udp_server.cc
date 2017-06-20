#include <iostream>

#include <core/app-template.hh>
#include <core/reactor.hh>

class UDPServer {
 public:
  UDPServer(ipv4_addr addr) : m_listen_addr{addr} {}
  future<> benchmark(int seconds);

  size_t m_packet_size;  // FIXME

 private:
  net::udp_channel m_chan;
  ipv4_addr m_listen_addr;
  timer<> m_bench_timer;
  std::chrono::seconds m_bench_duration;
  int m_bench_sec;
  uint64_t m_byte_cnt;
  bool m_is_timeout;
};

future<> UDPServer::benchmark(int seconds) {
  m_bench_sec = seconds;
  m_byte_cnt = 0;
  m_is_timeout = false;

  auto addr = make_ipv4_address(m_listen_addr);
  m_chan = engine().net().make_udp_channel(addr);

  m_bench_timer.set_callback([this]() mutable {
    m_chan.close();
    m_is_timeout = true;
  });
  m_bench_duration = std::chrono::seconds{seconds};

  return repeat([this]() mutable {
           if (m_is_timeout) {
             return make_ready_future<stop_iteration>(stop_iteration::yes);
           }

           return m_chan.receive().then([this](auto dgram) mutable {
             m_bench_timer.arm(m_bench_duration);
             m_byte_cnt += dgram.get_data().len();
             return m_chan.receive();
           }).then([this](auto dgram) mutable {
             m_byte_cnt += dgram.get_data().len();
             return make_ready_future<stop_iteration>(stop_iteration::no);
           });
         })
      .then([this]() mutable {
        auto mbit_cnt = 8 * m_byte_cnt / 1000000;
        std::cout << "duration sec, bytes sent, packet size, throughput, "
                     "throuput unit\n"
                  << m_bench_sec << ',' << m_byte_cnt << ',' << m_packet_size
                  << ',' << mbit_cnt / m_bench_sec << ',' << "10^6bits\n";

        return make_ready_future<>();
      });
}

future<> app_main(app_template& app) {
  auto& args = app.configuration();
  auto port = args["port"].as<uint16_t>();
  auto duration = args["duration"].as<int>();
  auto psize = args["psize"].as<size_t>();

  return do_with(UDPServer{port}, [duration, psize](auto& server) {
    server.m_packet_size = psize;  // FIXME
    return server.benchmark(duration);
  });
}

int main(int argc, char* argv[]) {
  namespace bpo = boost::program_options;

  app_template app;
  app.add_options()("port", bpo::value<uint16_t>()->default_value(12865),
                    "seaperf server port");
  app.add_options()("duration", bpo::value<int>()->default_value(10),
                    "benchmark duration");
  app.add_options()("psize", bpo::value<size_t>()->default_value(64),
                    "packet size");

  return app.run(argc, argv, [&app] { return app_main(app); });
}
