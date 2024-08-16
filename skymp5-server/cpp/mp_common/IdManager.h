#pragma once
#include "Networking.h"
#include "RakNet.h"
#include <unordered_map>
#include <vector>

class IdManager
{
public:
  using userid = Networking::UserId;

  IdManager(userid maxConnections_);

  userid allocateId(
    const RakNetGUID&
      guid) noexcept; // returns IServer::invalidUserId on failure
  void freeId(userid id) noexcept;
  userid find(const RakNetGUID& guid) const noexcept;
  RakNetGUID find(userid id) const noexcept;

private:
  std::unordered_map<uint64_t, userid> idByGuid;
  std::vector<RakNetGUID> guidById;
  userid nextId = 0;
  const userid maxConnections = 0;
};
