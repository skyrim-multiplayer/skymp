#pragma once
#pragma once
#include "NetworkingInterface.h"
#include <memory>
#include <vector>

namespace Networking {

using ServersVec = std::vector<std::shared_ptr<IServer>>;

std::shared_ptr<IServer> CreateCombinedServer(const ServersVec& childs);
}
