#pragma once
#include "NetworkingInterface.h"
#include <MakeID.h-1.0.2>
#include <fmt/format.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace Networking {
using ServersVec = std::vector<std::shared_ptr<IServer>>;

class ServerCombined : public Networking::IServer
{
public:
  ServerCombined(const Networking::ServersVec& childs_)
    : childs(childs_)
  {
    makeId.reset(new MakeID(std::numeric_limits<uint32_t>::max()));
    childData.resize(childs_.size());
  }

  void Send(Networking::UserId targetUserId, Networking::PacketData data,
            size_t length, bool reliable) override
  {
    if (realIdByCombined.size() <= targetUserId)
      throw std::runtime_error("User with id " + std::to_string(targetUserId) +
                               " doesn't exist");

    auto& p = realIdByCombined[targetUserId];
    auto serverIdx = p.first;
    auto userId = p.second;

    if (userId == Networking::InvalidUserId)
      throw std::runtime_error("User with id " + std::to_string(targetUserId) +
                               " doesn't exist");

    childs[serverIdx]->Send(userId, data, length, reliable);
  }

  void Tick(OnPacket onPacket, void* state) override
  {
    st.onPacket = onPacket;
    st.state = state;
    st.serverIdx = 0;
    for (auto& child : childs) {
      child->Tick(HandlePacket, this);
      ++st.serverIdx;
    }
  }

  Networking::UserId GetCombinedUserId(size_t serverIdx,
                                       Networking::UserId realUserId) const
  {
    if (serverIdx >= childData.size()) {
      throw std::runtime_error(fmt::format(
        "GetCombinedUserId: Bad server index {}, we only have {} servers",
        serverIdx, childData.size()));
    }
    auto& data = childData[serverIdx];

    if (realUserId >= data.combinedIdByReal.size()) {
      return Networking::InvalidUserId; // not found
    }

    return data.combinedIdByReal[realUserId];
  }

private:
  static void HandlePacket(void* state, Networking::UserId userId,
                           Networking::PacketType packetType,
                           Networking::PacketData data, size_t length)
  {
    auto this_ = reinterpret_cast<ServerCombined*>(state);

    const auto serverIdx = this_->st.serverIdx;

    auto& combinedIdByReal = this_->childData[serverIdx].combinedIdByReal;
    Networking::UserId id = Networking::InvalidUserId;

    switch (packetType) {
      case Networking::PacketType::ServerSideUserConnect: {

        id = this_->CreateId();

        if (combinedIdByReal.size() <= userId) {
          combinedIdByReal.resize(static_cast<size_t>(userId) + 1,
                                  Networking::InvalidUserId);
        }
        combinedIdByReal[userId] = id;

        if (this_->realIdByCombined.size() <= id) {
          this_->realIdByCombined.resize(static_cast<size_t>(id) + 1,
                                         { -1, Networking::InvalidUserId });
        }
        this_->realIdByCombined[id] = { this_->st.serverIdx, userId };

        break;
      }
      case Networking::PacketType::ServerSideUserDisconnect:
        id = combinedIdByReal[userId];
        this_->FreeId(id);
        combinedIdByReal[userId] = Networking::InvalidUserId;
        this_->realIdByCombined[id] = { -1, Networking::InvalidUserId };
        break;
      case Networking::PacketType::Message:
        id = combinedIdByReal[userId];
        break;
      default:
        break;
    }

    this_->st.onPacket(this_->st.state, id, packetType, data, length);
  }

  Networking::UserId CreateId()
  {
    uint32_t id;
    if (!makeId->CreateID(id))
      std::terminate();
    return id;
  }

  void FreeId(Networking::UserId id)
  {
    if (!makeId->DestroyID(id))
      std::terminate();
  }

  const Networking::ServersVec childs;

  struct
  {
    OnPacket onPacket = nullptr;
    void* state = nullptr;
    size_t serverIdx = ~0;
  } st;

  struct ChildData
  {
    std::vector<Networking::UserId> combinedIdByReal;
  };

  std::vector<ChildData> childData;

  std::unique_ptr<MakeID> makeId;

  std::vector<std::pair<size_t, Networking::UserId>> realIdByCombined;
};

std::shared_ptr<ServerCombined> CreateCombinedServer(const ServersVec& childs);
}
