#pragma once
#include "../server_guest_lib/AnimationData.h"
#include "../server_guest_lib/Appearance.h"
#include "../server_guest_lib/Equipment.h"
#include "../server_guest_lib/Inventory.h"
#include "MessageBase.h"
#include "MsgType.h"
#include <map>
#include <type_traits>

struct Transform
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("worldOrCell", worldOrCell)
      .Serialize("pos", pos)
      .Serialize("rot", rot);
  }

  uint32_t worldOrCell = 0;
  std::array<float, 3> pos = { 0.f, 0.f, 0.f };
  std::array<float, 3> rot = { 0.f, 0.f, 0.f };
};

struct SetNodeTextureSetEntry
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("nodeName", nodeName)
      .Serialize("textureSetId", textureSetId);
  }

  std::string nodeName;
  uint32_t textureSetId = 0;
};

struct SetNodeScaleEntry
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("nodeName", nodeName).Serialize("scale", scale);
  }

  std::string nodeName;
  float scale = 0.f;
};

struct CustomPropsEntry
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("propName", propName)
      .Serialize("propValueJsonDump", propValueJsonDump);
  }

  std::string propName;
  std::string propValueJsonDump;
};

struct CreateActorMessageAdditionalProps
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("isOpen", isOpen)
      .Serialize("setNodeTextureSet", setNodeTextureSet)
      .Serialize("setNodeScale", setNodeScale)
      .Serialize("isDisabled", isDisabled)
      .Serialize("lastAnimation", lastAnimation)
      .Serialize("displayName", displayName)
      .Serialize("isHostedByOther", isHostedByOther)
      .Serialize("isRaceMenuOpen", isRaceMenuOpen)
      .Serialize("learnedSpells", learnedSpells)
      .Serialize("healRate", healRate)
      .Serialize("healRateMult", healRateMult)
      .Serialize("health", health)
      .Serialize("magickaRate", magickaRate)
      .Serialize("magickaRateMult", magickaRateMult)
      .Serialize("magicka", magicka)
      .Serialize("staminaRate", staminaRate)
      .Serialize("staminaRateMult", staminaRateMult)
      .Serialize("stamina", stamina)
      .Serialize("healthPercentage", healthPercentage)
      .Serialize("staminaPercentage", staminaPercentage)
      .Serialize("magickaPercentage", magickaPercentage)
      .Serialize("templateChain", templateChain)
      .Serialize("inventory", inventory)
      .Serialize("isDead", isDead);
  }

  std::optional<bool> isOpen;
  std::optional<std::vector<SetNodeTextureSetEntry>> setNodeTextureSet;
  std::optional<std::vector<SetNodeScaleEntry>> setNodeScale;
  std::optional<bool> isDisabled;
  std::optional<std::string> lastAnimation;
  std::optional<std::string> displayName;
  std::optional<bool> isHostedByOther;
  std::optional<bool> isRaceMenuOpen;
  std::optional<std::vector<uint32_t>> learnedSpells;
  std::optional<float> healRate;
  std::optional<float> healRateMult;
  std::optional<float> health;
  std::optional<float> magickaRate;
  std::optional<float> magickaRateMult;
  std::optional<float> magicka;
  std::optional<float> staminaRate;
  std::optional<float> staminaRateMult;
  std::optional<float> stamina;
  std::optional<float> healthPercentage;
  std::optional<float> staminaPercentage;
  std::optional<float> magickaPercentage;
  std::optional<std::vector<uint32_t>> templateChain;
  std::optional<Inventory> inventory;

  // TODO: take a look why doubles CreateActorMessageMainProps
  std::optional<bool> isDead;
};

struct CreateActorMessageMainProps
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("refrId", refrId)
      .Serialize("baseId", baseId)
      .Serialize("appearance", appearance)
      .Serialize("equipment", equipment)
      .Serialize("animation", animation)
      .Serialize("isDead", isDead);
  }

  std::optional<uint64_t> refrId;
  std::optional<uint32_t> baseId = 0;
  std::optional<Appearance> appearance;
  std::optional<Equipment> equipment;
  std::optional<AnimationData> animation;
  std::optional<bool> isDead;
};

struct CreateActorMessage
  : public MessageBase<CreateActorMessage>
  , public CreateActorMessageMainProps
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::CreateActor)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("idx", idx)
      .Serialize("baseRecordType", baseRecordType)
      .Serialize("transform", transform)
      .Serialize("isMe", isMe)
      .Serialize("props", props)
      .Serialize("customPropsJsonDumps", customPropsJsonDumps);

    CreateActorMessageMainProps::Serialize(archive);
  }

  uint32_t idx = 0;
  std::optional<std::string> baseRecordType;
  Transform transform;
  bool isMe = false;
  CreateActorMessageAdditionalProps props;
  std::vector<CustomPropsEntry> customPropsJsonDumps;
};
