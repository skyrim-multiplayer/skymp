#include <Networking.h>
#include <cr.h>
#include <cstdio>
#include <iostream>
#include <memory>

static unsigned int CR_STATE g_version = 1;

namespace {
class Skymp5Server
{
public:
  Skymp5Server()
  {
    auto port = 7777;
    auto maxPlayers = 1000;
    server = Networking::CreateServer(port, maxPlayers);
    std::cout << "Listening on " << port << std::endl;
  }
  ~Skymp5Server() {}

  void Tick()
  {
    server->Tick(
      [](void* state, Networking::UserId userId,
         Networking::PacketType packetType, Networking::PacketData data,
         size_t length) {
        auto this_ = reinterpret_cast<Skymp5Server*>(state);

        switch (packetType) {
          case Networking::PacketType::ServerSideUserConnect:
            std::cout << "Connected " << userId << std::endl;
            break;
          case Networking::PacketType::ServerSideUserDisconnect:
            std::cout << "Disconnected " << userId << std::endl;
            break;
          case Networking::PacketType::Message:
            std::string str(reinterpret_cast<const char*>(data + 1),
                            length - 1);
            std::cout << "Message from " << userId << ": " << str << std::endl;
            this_->server->Send(userId, data, length, false);
            break;
        }
      },
      this);
  }

private:
  std::shared_ptr<Networking::IServer> server;
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