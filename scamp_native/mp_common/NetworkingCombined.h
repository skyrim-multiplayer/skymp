#pragma once
#pragma once
#include "NetworkingInterface.h"
#include <memory>
#include <vector>

namespace Networking {

using ServersVec = std::vector<std::shared_ptr<IServer>>;

/// Create server from children
/// \param children Children of server
/// \return New united server
std::shared_ptr<IServer> CreateCombinedServer(const ServersVec& children);
}