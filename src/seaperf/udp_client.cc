#include <iostream>

#include <core/app-template.hh>
#include <core/reactor.hh>

#include "utils.hh"

class UDPClient {
 public:
  UDPClient(ipv4_addr addr) : m_server_addr{addr} {}
  future<> benchmark(int seconds, size_t psize);

  int m_packet_size;  // FIXME

 private:
  net::udp_channel m_chan;
  ipv4_addr m_server_addr;
  std::string m_sendbuf;
  timer<> m_bench_timer;
  bool m_is_timeout;
  static constexpr int k_margin_sec = 4;
};

future<> UDPClient::benchmark(int seconds, size_t psize) {
  m_is_timeout = false;
  m_sendbuf = seaperf::random_string(psize);

  m_chan = engine().net().make_udp_channel();

  m_bench_timer.set_callback([this]() mutable {
    m_chan.close();
    m_is_timeout = true;
  });
  auto bench_duration = std::chrono::seconds{seconds + k_margin_sec};
  m_bench_timer.arm(bench_duration);

  return repeat([this]() mutable {
           if (m_is_timeout) {
             return make_ready_future<stop_iteration>(stop_iteration::yes);
           }

           return m_chan.send(m_server_addr, m_sendbuf.c_str()).then([this] {
             return make_ready_future<stop_iteration>(stop_iteration::no);
           });
         })
      .then([this]() mutable { return make_ready_future<>(); });
}

future<> app_main(app_template& app) {
  auto& args = app.configuration();
  auto host = args["host"].as<std::string>();
  auto port = args["port"].as<uint16_t>();
  auto duration = args["duration"].as<int>();
  auto psize = args["psize"].as<size_t>();
  ipv4_addr server_addr{host, port};

  return do_with(UDPClient{server_addr}, [duration, psize](auto& server) {
    return server.benchmark(duration, psize);
  });
}

int main(int argc, char* argv[]) {
  namespace bpo = boost::program_options;

  app_template app;
  app.add_options()("host",
                    bpo::value<std::string>()->default_value("127.0.0.1"),
                    "seaperf server host");
  app.add_options()("port", bpo::value<uint16_t>()->default_value(12865),
                    "seaperf server port");
  app.add_options()("duration", bpo::value<int>()->default_value(10),
                    "benchmark duration");
  app.add_options()("psize", bpo::value<size_t>()->default_value(64),
                    "packet size");

  return app.run(argc, argv, [&app] { return app_main(app); });
}
