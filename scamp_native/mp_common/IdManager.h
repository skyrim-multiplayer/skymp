#pragma once
#include "Networking.h"
#include "RakNet.h"
#include <map>
#include <sparsepp/spp.h>
#include <vector>

class IdManager
{
public:
  using userid = Networking::UserId;

  IdManager(userid maxConnections_);


  /// \return IServer::invalidUserId on failure
  userid allocateId(
    const RakNetGUID&
      guid) noexcept;
  void freeId(userid id) noexcept;
  userid find(const RakNetGUID& guid) const noexcept;
  RakNetGUID find(userid id) const noexcept;

private:
  spp::sparse_hash_map<uint64_t, userid> idByGuid;
  std::vector<RakNetGUID> guidById;
  userid nextId = 0;
  const userid maxConnections = 0;
};