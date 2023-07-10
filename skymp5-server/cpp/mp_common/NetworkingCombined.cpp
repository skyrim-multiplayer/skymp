#include "NetworkingCombined.h"

std::shared_ptr<Networking::ServerCombined> Networking::CreateCombinedServer(
  const ServersVec& childs)
{
  return std::make_shared<ServerCombined>(childs);
}
