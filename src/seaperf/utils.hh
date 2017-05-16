#include <string>

#include <core/reactor.hh>

namespace seaperf {

future<connected_socket> stubborn_connect(ipv4_addr addr);
std::string random_string(std::size_t len);
}
