#include <iostream>
#include <stdexcept>

#include <core/app-template.hh>
#include <core/distributed.hh>
#include <core/future.hh>
#include <core/reactor.hh>
#include <net/api.hh>
#include <util/log.hh>

#include "server.hh"

using Server = seaperf::server::Server;

void app_main(app_template& app) {
  auto& args = app.configuration();
  auto port = args["port"].as<uint16_t>();
  auto once = args["once"].as<bool>();
  listen_options lo;
  lo.reuse_address = true;
  auto server = new Server{};

  server->start()
      .then(
          [server, port, once]() mutable { return server->listen(port, once); })
      .then([server, port]() mutable {
        print("Seaperf server listening on port %d ...\n", port);
        engine().at_exit([server]() mutable {
          return server->stop();
        });
      });
}

int main(int argc, char* argv[]) {
  namespace bpo = boost::program_options;

  app_template app;
  app.add_options()("port", bpo::value<uint16_t>()->default_value(12865),
                    "seaperf server port");
  app.add_options()("once", bpo::bool_switch()->default_value(false),
                    "only accept a single connection");

  return app.run_deprecated(argc, argv, [&app] { return app_main(app); });
}
