#include "TestUtils.hpp"
#include "FormCallbacks.h"
#include "IPapyrusCompatibilityPolicy.h"
#include "MpActor.h"
#include "MsgType.h"
#include "PartOne.h"
#include <catch2/catch_all.hpp>

// Utilities for testing

bool IsCmakeOptionSpecified(const std::string& optionValue)
{
  return !optionValue.empty() && optionValue != "OFF";
}

const char* GetDataDir()
{
  return IsCmakeOptionSpecified(UNIT_DATA_DIR) ? UNIT_DATA_DIR
                                               : SKYRIM_DIR "/Data";
}

std::string MakeMessage(const nlohmann::json& j)
{
  std::string s;
  s += (char)Networking::MinPacketId;
  s += j.dump();
  return s;
}

void DoMessage(PartOne& partOne, Networking::UserId id,
               const nlohmann::json& j)
{
  std::string s = MakeMessage(j);
  PartOne* ptr = &partOne;
  PartOne::HandlePacket(ptr, id, Networking::PacketType::Message,
                        reinterpret_cast<Networking::PacketData>(s.data()),
                        s.size());
}

void DoConnect(PartOne& partOne, Networking::UserId id)
{
  PartOne* ptr = &partOne;
  PartOne::HandlePacket(ptr, id, Networking::PacketType::ServerSideUserConnect,
                        nullptr, 0);
}

void DoDisconnect(PartOne& partOne, Networking::UserId id)
{
  PartOne* ptr = &partOne;
  PartOne::HandlePacket(
    ptr, id, Networking::PacketType::ServerSideUserDisconnect, nullptr, 0);
}

void DoUpdateMovement(PartOne& partOne, uint32_t actorFormId,
                      Networking::UserId userId)
{
  auto jMyMovement = jMovement;
  jMyMovement["idx"] = dynamic_cast<MpActor*>(
                         partOne.worldState.LookupFormById(actorFormId).get())
                         ->GetIdx();
  DoMessage(partOne, userId, jMyMovement);
}

std::shared_ptr<FakeListener> FakeListener::New()
{
  return std::shared_ptr<FakeListener>(new FakeListener);
}

void FakeListener::OnConnect(Networking::UserId userId)
{
  ss << "OnConnect(" << userId << ")" << std::endl;
}

void FakeListener::OnDisconnect(Networking::UserId userId)
{
  ss << "OnDisconnect(" << userId << ")" << std::endl;
}

void FakeListener::OnCustomPacket(Networking::UserId userId,
                                  const simdjson::dom::element& content)
{
  ss << "OnCustomPacket(" << userId << ", " << simdjson::minify(content) << ")"
     << std::endl;
}

bool FakeListener::OnMpApiEvent(const char* eventName,
                                std::optional<simdjson::dom::element> args,
                                std::optional<uint32_t> formId)
{
  return true;
}

std::string FakeListener::str()
{
  return ss.str();
}

void FakeListener::clear()
{
  ss = std::stringstream();
}

PapyrusCompatibilityPolicy::PapyrusCompatibilityPolicy(MpActor* ac_)
  : ac(ac_)
{
}

MpActor* PapyrusCompatibilityPolicy::GetDefaultActor(const char*, const char*,
                                                     int32_t) const
{
  return ac;
}
