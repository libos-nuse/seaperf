#include <algorithm>
#include <chrono>
#include <experimental/optional>
#include <iterator>
#include <random>
#include <sstream>
#include <string>

#include <core/reactor.hh>
#include <core/sleep.hh>

#include "utils.hh"

namespace seaperf {

future<connected_socket> stubborn_connect(ipv4_addr addr) {
  using sock_opt = std::experimental::optional<connected_socket>;
  using namespace std::chrono_literals;

  return repeat_until_value([=] {
    return engine().connect(addr).then_wrapped([](auto f_sock) {
      if (f_sock.failed()) {
        return sleep(3s).then([] {
          return make_ready_future<sock_opt>();
        });
      } else {
        return make_ready_future<sock_opt>(f_sock.get());
      }
    });
  });
}

std::string random_string(std::size_t len) {
  auto randchar = []() -> char {
    const char charset[] = "abcdefghijklmnopqrstuvwxyz";
    std::random_device rnd;
    return charset[rnd() % sizeof(charset)];
  };

  std::ostringstream rand_ss;
  std::ostream_iterator<char> rand_os_it{rand_ss};
  std::generate_n(rand_os_it, len, randchar);
  return rand_ss.str();
}
}  // namespace seaperf
