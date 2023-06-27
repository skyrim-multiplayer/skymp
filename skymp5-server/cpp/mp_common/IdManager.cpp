#include "IdManager.h"

using userid = IdManager::userid;

IdManager::IdManager(userid maxConnections_)
  : maxConnections(maxConnections_)
{
  guidById.resize(maxConnections_, UNASSIGNED_RAKNET_GUID);
}

userid IdManager::allocateId(const RakNetGUID& guid) noexcept
{
  auto& idByGuid = this->idByGuid[guid.g];
  if (nextId >= maxConnections) {
    return Networking::InvalidUserId;
  }
  const userid res = nextId++;
  if (nextId < maxConnections) {
    while (guidById[nextId] != UNASSIGNED_RAKNET_GUID) {
      if (++nextId >= maxConnections) {
        break;
      }
    }
  }
  guidById[res] = guid;
  idByGuid = res;
  return res;
}

void IdManager::freeId(userid id) noexcept
{
  const auto guid = guidById[id];
  guidById[id] = UNASSIGNED_RAKNET_GUID;
  idByGuid.erase(guid.g);
  if (id < nextId) {
    nextId = id;
  }
}

userid IdManager::find(const RakNetGUID& guid) const noexcept
{
  auto it = idByGuid.find(guid.g);
  if (it == idByGuid.end()) {
    return Networking::InvalidUserId;
  }
  return it->second;
}

RakNetGUID IdManager::find(userid id) const noexcept
{
  if (id < maxConnections) {
    return guidById[id];
  }
  return UNASSIGNED_RAKNET_GUID;
}
