#pragma once
#include "FormCallbacks.h"
#include "MpActor.h"
#include "MsgType.h"
#include "PartOne.h"
#include "script_compatibility_policies/IPapyrusCompatibilityPolicy.h"
#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>
#include <thread>

// Utilities for testing

enum class Constants : uint32_t
{
  kWhiterunCell = 0x1a26f,
  kBarrelInWhiterun = 0x4cc2d,
  kTamriel = 0x3c
};

std::underlying_type_t<Constants> ToUnderlying(Constants constant);

bool IsCmakeOptionSpecified(const std::string& optionValue);

const char* GetDataDir();

std::string MakeMessage(const nlohmann::json& j);

void DoMessage(PartOne& partOne, Networking::UserId id,
               const nlohmann::json& j);

void DoConnect(PartOne& partOne, Networking::UserId id);

void DoDisconnect(PartOne& partOne, Networking::UserId id);

void DoUpdateMovement(PartOne& partOne, uint32_t actorFormId,
                      Networking::UserId userId);

static const auto jMovement =
  nlohmann::json{ { "t", MsgType::UpdateMovement },
                  { "idx", 0 },
                  { "data",
                    { { "worldOrCell", 0x3c },
                      { "pos", { 1.f, -1.f, 1.f } },
                      { "rot", { 0.f, 0.f, 179.f } },
                      { "runMode", "Standing" },
                      { "direction", 0.f },
                      { "healthPercentage", 1.f },
                      { "speed", 0.f },
                      { "isInJumpState", false },
                      { "isSneaking", false },
                      { "isBlocking", false },
                      { "isDead", false },
                      { "isWeapDrawn", false } } } };

static const auto jAppearance = nlohmann::json{
  { "t", MsgType::UpdateAppearance },
  { "idx", 0 },
  { "data",
    { { "isFemale", false },
      { "raceId", 0x00000001 },
      { "weight", 99.9f },
      { "skinColor", -1 },
      { "hairColor", -1 },
      { "headpartIds", nlohmann::json::array() },
      { "headTextureSetId", 0x00000000 },
      { "tints", nlohmann::json::array() },
      { "name", "Oberyn" },
      { "options",
        nlohmann::json::array({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0 }) },                  // size=19
      { "presets", nlohmann::json::array({ 0, 0, 0, 0 }) } } } // size=4
};

static const auto jEquipment = nlohmann::json{
  { "t", MsgType::UpdateEquipment },
  { "idx", 0 },
  { "data",
    { { "numChanges", 0 },
      { "inv", { { "entries", nlohmann::json::array() } } } } }
};

class FakeListener : public PartOne::Listener
{
public:
  static std::shared_ptr<FakeListener> New();

  void OnConnect(Networking::UserId userId) override;

  void OnDisconnect(Networking::UserId userId) override;

  void OnCustomPacket(Networking::UserId userId,
                      const simdjson::dom::element& content) override;

  bool OnMpApiEvent(const GameModeEvent& event) override;

  std::string str();

  void clear();

private:
  std::stringstream ss;
};

namespace {
template <class Message>
struct FindRefrMessageResult
{
  std::vector<Message> filteredMessages;
  std::vector<PartOne::Message> filteredMessagesOriginals;

  FindRefrMessageResult(
    const std::vector<Message>& filteredMessages,
    const std::vector<PartOne::Message>& filteredMessagesOriginals)
    : filteredMessages(filteredMessages)
    , filteredMessagesOriginals(filteredMessagesOriginals)
  {
  }
};

// Deduction guide for C++17 and later
#if __cplusplus >= 201703L
// This allows: FindRefrMessageResult{vec1, vec2} to deduce Message type
// (vec1: std::vector<Message>, vec2: std::vector<PartOne::Message>)
template <class Message>
FindRefrMessageResult(const std::vector<Message>&,
                      const std::vector<PartOne::Message>&)
  -> FindRefrMessageResult<Message>;
#endif

template <class Message>
inline FindRefrMessageResult<Message> FindRefrMessage(PartOne& partOne,
                                                      uint32_t expectedRefrId)
{
  std::vector<Message> filteredMessages;
  std::vector<PartOne::Message> filteredMessagesOriginals;

  for (auto& message : partOne.Messages()) {
    if (auto createActorMessage =
          dynamic_cast<Message*>(message.message.get())) {
      if (createActorMessage->refrId == expectedRefrId) {
        filteredMessages.push_back(*createActorMessage);
        filteredMessagesOriginals.push_back(message);
      }
    }
  }

  return FindRefrMessageResult{ filteredMessages, filteredMessagesOriginals };
}

template <class Message>
inline FindRefrMessageResult<Message> FindRefrMessageIdx(PartOne& partOne,
                                                         int expectedIdx)
{
  std::vector<Message> filteredMessages;
  std::vector<PartOne::Message> filteredMessagesOriginals;

  for (auto& message : partOne.Messages()) {
    if (auto createActorMessage =
          dynamic_cast<Message*>(message.message.get())) {
      if (createActorMessage->idx == expectedIdx) {
        filteredMessages.push_back(*createActorMessage);
        filteredMessagesOriginals.push_back(message);
      }
    }
  }

  return FindRefrMessageResult{ filteredMessages, filteredMessagesOriginals };
}
}
