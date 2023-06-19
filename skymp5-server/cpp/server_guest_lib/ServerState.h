#pragma once
#include "ActorsMap.h"
#include "Config.h"
#include <Networking.h>
#include <array>
#include <chrono>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <simdjson.h>
#include <unordered_map>

class MpActor;

struct PacketHistoryElement
{
  size_t offset = 0;
  size_t length = 0;
  uint64_t timeMs = 0;
};

struct PacketHistory
{
  std::vector<uint8_t> buffer;
  std::deque<PacketHistoryElement> packets;
};

struct Playback
{
  PacketHistory history;
  std::chrono::time_point<std::chrono::steady_clock> startTime;
};

struct UserInfo
{
  bool isDisconnecting = false;

  bool isPacketHistoryRecording = false;
  PacketHistory packetHistory;
  std::optional<std::chrono::time_point<std::chrono::steady_clock>>
    packetHistoryStartTime;
};

class ServerState
{
public:
  ServerState() { userInfo.resize(kMaxPlayers); }

  std::vector<std::unique_ptr<UserInfo>> userInfo;
  Networking::UserId maxConnectedId = 0;
  ActorsMap actorsMap;
  Networking::UserId disconnectingUserId = Networking::InvalidUserId;
  std::map<Networking::UserId, Playback>
    activePlaybacks; // do not modify directly, use requestedPlaybacks
  std::map<Networking::UserId, Playback> requestedPlaybacks;

  void Connect(Networking::UserId userId);
  void Disconnect(Networking::UserId userId) noexcept;
  bool IsConnected(Networking::UserId userId) const;
  MpActor* ActorByUser(Networking::UserId userId);
  Networking::UserId UserByActor(MpActor* actor);
  void EnsureUserExists(Networking::UserId userId);
};
