#include <algorithm>
#include <string>

#include <core/byteorder.hh>

#include "benchmark.hh"
#include "client.hh"
#include "utils.hh"

namespace seaperf {
namespace client {

Client::Client() {}

future<> Client::benchmark() {
  return repeat([this] {
    if (m_is_time_up) {
      return make_ready_future<stop_iteration>(stop_iteration::yes);
    }
    return m_out.write(m_sendbuf).then_wrapped([this](auto&& f) {
      try {
        f.get();
      } catch (std::exception& ex) {
        std::cerr << "write error: " << ex.what() << std::endl;
        return make_ready_future<stop_iteration>(stop_iteration::yes);
      }
      return make_ready_future<stop_iteration>(stop_iteration::no);
    });
  });
}

future<> Client::run(ipv4_addr addr) {
  m_sendbuf = random_string(m_sendbuf_size);

  m_is_time_up = false;
  m_bench_timer.set_callback([this] { m_is_time_up = true; });

  return stubborn_connect(addr)
      .then([this](auto sock) mutable {
        m_sock = std::move(sock);
        m_in = m_sock.input();
        m_out = m_sock.output();

        BenchmarkRequest req;
        req.duration = cpu_to_le(m_bench_duration.count());
        req.packet_size = cpu_to_le(m_sendbuf_size);
        return m_out.write(reinterpret_cast<char*>(&req), sizeof(req));
      })
      .then([this]() mutable {
        m_bench_timer.arm(m_bench_duration + m_margin);
        return this->benchmark();
      })
      .then([this]() mutable {
        return m_in.read_exactly(sizeof(BenchmarkResult));
      })
      .then([this](auto buf) mutable {
        BenchmarkResult res = *reinterpret_cast<const BenchmarkResult*>(buf.get());
        res.byte_cnt = le_to_cpu(res.byte_cnt);
        auto bench_sec = m_bench_duration.count();
        auto mbit_cnt = 8 * res.byte_cnt / 1000000;
        std::cout << "duration sec, bytes sent, packet size, throughput, "
                     "throuput unit\n"
                  << bench_sec << ',' << res.byte_cnt << ',' << m_sendbuf_size
                  << ',' << mbit_cnt / bench_sec << ',' << "10^6bits\n";
      })
      .then([this]() mutable {
        return when_all(m_in.close(), m_out.close()).discard_result();
      });
}

void Client::set_bench_duration(std::chrono::seconds t) {
  m_bench_duration = t;
}

void Client::set_bufsize(size_t bufsize) { m_sendbuf_size = bufsize; }
}  // namespace client
}  // namespace seaperf
