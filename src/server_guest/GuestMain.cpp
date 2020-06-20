#include "Networking.h"
#include "P2.h"
#include "P2SessionManager.h"
#include "PartOne.h"
#include <cr.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <mutex>
#include <spdlog/sinks/stdout_color_sinks.h>

static unsigned int CR_STATE g_version = 1;

namespace {
class Skymp5Server
{
public:
  Skymp5Server()
  {
    partTwo.reset(new P2);
    partOne.reset(new PartOne);
    P2::Attach(partOne, partTwo);

    auto port = 7777;
    auto maxPlayers = 1000;
    server = Networking::CreateServer(port, maxPlayers);
    logger->info("Listening on {}", port);
  }

  void Tick()
  {
    while (1) {
      try {
        return server->Tick(PartOne::HandlePacket, partOne.get());
      } catch (std::exception& e) {
        logger->error(e.what());
      }
    }
  }

private:
  std::shared_ptr<Networking::IServer> server;
  std::shared_ptr<PartOne> partOne;
  std::shared_ptr<P2> partTwo;
  std::shared_ptr<spdlog::logger> logger;
};
}

static std::unique_ptr<Skymp5Server> g_skymp5server;

CR_EXPORT int cr_main(struct cr_plugin* ctx, enum cr_op operation)
{
  switch (operation) {
    case CR_LOAD:
      std::cout << "CR: Load" << std::endl;
      g_skymp5server.reset(new Skymp5Server);
      break;
    case CR_STEP:
      // crash protection may cause the version to decrement. So we can test
      // current version against one tracked between instances with CR_STATE to
      // signal that we're not running the most recent instance.
      if (ctx->version < g_version) {
        // a failure code is acessible in the `failure` variable from the
        // `cr_plugin` context. on windows this is the structured exception
        // error code, for more info:
        //      https://msdn.microsoft.com/en-us/library/windows/desktop/ms679356(v=vs.85).aspx
        fprintf(stdout, "A rollback happened due to failure: %x!\n",
                ctx->failure);
      }
      g_version = ctx->version;
      g_skymp5server->Tick();
      break;
    case CR_UNLOAD:
      std::cout << "CR: Unload" << std::endl;
      g_skymp5server.reset();
      break;
    case CR_CLOSE:
      std::cout << "CR: Close" << std::endl;
      g_skymp5server.reset();
      break;
  }

  return 0;
}