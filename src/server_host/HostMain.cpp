#define CR_HOST
#include <cr.h>
#include <filesystem>
#include <stdexcept>

int main(int argc, char* argv[])
{
  cr_plugin ctx;

  fprintf(stdout, "argv[0] is '%s'\n", argv[0]);

  static const std::string guestPath =
    (std::filesystem::path(argv[0]).parent_path() / CR_PLUGIN("server_guest"))
      .string();
  if (!cr_plugin_open(ctx, guestPath.data())) {
    fprintf(stderr, "failed to open '%s'\n", guestPath.data());
    return -1;
  }

  while (!cr_plugin_update(ctx)) {
    fflush(stdout);
    fflush(stderr);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  cr_plugin_close(ctx);
  return 0;
}

/*int main()
{
  try {
    auto port = 7777;
    auto maxPlayers = 1000;
    static auto server = Networking::CreateServer(port, maxPlayers);
    std::cout << "Listening on " << port << std::endl;
    while (1) {
      server->Tick(
        [](void* state, Networking::UserId userId,
           Networking::PacketType packetType, Networking::PacketData data,
           size_t length) {
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
              std::cout << "Message from " << userId << ": " << str
                        << std::endl;
              server->Send(userId, data, length, false);
              break;
          }
        },
        nullptr);

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return 0;
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return -1;
  }
}*/