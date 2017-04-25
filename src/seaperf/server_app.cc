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
  listen_options lo;
  lo.reuse_address = true;
  auto server = new seaperf::server::Server{};
  server->start()
      .then([server, port] { return server->listen(port); })
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

  try {
    app.run(argc, argv, [&app] { return app_main(app); });
  } catch (...) {
    std::cerr << std::current_exception() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
