#include <iostream>
#include <stdexcept>

#include <core/app-template.hh>
#include <core/distributed.hh>
#include <core/future.hh>
#include <core/reactor.hh>
#include <net/api.hh>
#include <util/log.hh>

#include "server.hh"

future<> app_main(app_template& app) {
  auto& args = app.configuration();
  auto port = args["port"].as<uint16_t>();
  auto duration_sec = args["duration"].as<uint64_t>();
  listen_options lo;
  lo.reuse_address = true;
  auto server = new seaperf::server::Server{};
  server->start()
      .then([server, port, duration_sec] { return server->listen(port, duration_sec); })
      .then([server, port] {
        print("Seaperf server listening on port %d ...\n", port);
        engine().at_exit([server] { return server->stop(); });
      });
  return server->stopped();
}

int main(int argc, char* argv[]) {
  namespace bpo = boost::program_options;

  app_template app;
  app.add_options()("port", bpo::value<uint16_t>()->default_value(12865),
                    "seaperf server port");
  app.add_options()("duration", bpo::value<uint64_t>()->default_value(10),
                    "benchmark duration in seconds");

  try {
    app.run(argc, argv, [&app] { return app_main(app); });
  } catch (...) {
    std::cerr << std::current_exception() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
