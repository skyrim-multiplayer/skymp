#include "MpObjectReference.h"
#include "ChangeFormGuard.h"
#include "EvaluateTemplate.h"
#include "FormCallbacks.h"
#include "GetWeightFromRecord.h"
#include "Inventory.h"
#include "LeveledListUtils.h"
#include "MathUtils.h"
#include "MpActor.h"
#include "MpChangeForms.h"
#include "MsgType.h"
#include "Primitive.h"
#include "ScopedTask.h"
#include "ScriptVariablesHolder.h"
#include "TimeUtils.h"
#include "WorldState.h"
#include "libespm/CompressedFieldsCache.h"
#include "libespm/Convert.h"
#include "libespm/GroupUtils.h"
#include "libespm/Utils.h"
#include "papyrus-vm/Reader.h"
#include "papyrus-vm/VirtualMachine.h"
#include "script_objects/EspmGameObject.h"
#include "script_storages/IScriptStorage.h"
#include <map>
#include <numeric>
#include <optional>

#include "OpenContainerMessage.h"
#include "TeleportMessage.h"

#include "script_classes/PapyrusObjectReference.h" // kOriginalNameExpression

constexpr uint32_t kPlayerCharacterLevel = 1;

UpdatePropertyMessage MpObjectReference::CreatePropertyMessage(
  MpObjectReference* self, const char* name, const nlohmann::json& value)
{
  return PreparePropertyMessage(self, name, value);
}

UpdatePropertyMessage MpObjectReference::PreparePropertyMessage(
  MpObjectReference* self, const char* name, const nlohmann::json& value)
{
  UpdatePropertyMessage res;

  std::string baseRecordType;

  auto& loader = self->GetParent()->GetEspm();
  auto base = loader.GetBrowser().LookupById(GetBaseId());
  if (base.rec) {
    baseRecordType = base.rec->GetType().ToString();
  }

  res.idx = self->GetIdx();
  res.propName = name;
  res.refrId = self->GetFormId();
  res.data = value;

  // See 'perf: improve game framerate #1186'
  // Client needs to know if it is DOOR or not
  if (baseRecordType == "DOOR") {
    res.baseRecordType = baseRecordType;
  }

  return res;
}

class OccupantDestroyEventSink : public MpActor::DestroyEventSink
{
public:
  OccupantDestroyEventSink(WorldState& wst_,
                           MpObjectReference* untrustedRefPtr_)
    : wst(wst_)
    , untrustedRefPtr(untrustedRefPtr_)
    , refId(untrustedRefPtr_->GetFormId())
  {
  }

  void BeforeDestroy(MpActor& actor) override
  {
    if (!RefStillValid())
      return;
    if (untrustedRefPtr->occupant == &actor) {
      untrustedRefPtr->SetOpen(false);
      untrustedRefPtr->occupant = nullptr;
    }
  }

private:
  bool RefStillValid() const
  {
    return untrustedRefPtr == wst.LookupFormById(refId).get();
  }

  WorldState& wst;
  MpObjectReference* const untrustedRefPtr;
  const uint32_t refId;
};

namespace {
std::pair<int16_t, int16_t> GetGridPos(const NiPoint3& pos) noexcept
{
  return { int16_t(pos.x / 4096), int16_t(pos.y / 4096) };
}
}

struct AnimGraphHolder
{
  std::set<CIString> animationVariablesBool;
};

struct ScriptState
{
  std::map<std::string, std::shared_ptr<ScriptVariablesHolder>> varHolders;
};

struct PrimitiveData
{
  NiPoint3 boundsDiv2;
  GeoProc::GeoPolygonProc polygonProc;
};

struct MpObjectReference::Impl
{
public:
  bool onInitEventSent = false;
  bool scriptsInited = false;
  std::unique_ptr<ScriptState> scriptState;
  std::unique_ptr<AnimGraphHolder> animGraphHolder;
  std::optional<PrimitiveData> primitive;
  bool teleportFlag = false;
  bool setPropertyCalled = false;
};

namespace {
auto MakeMode(bool isLocationSaveNeeded)
{
  return isLocationSaveNeeded ? ChangeFormGuard::Mode::RequestSave
                              : ChangeFormGuard::Mode::NoRequestSave;
}
MpChangeForm MakeChangeForm(const LocationalData& locationalData)
{
  MpChangeForm changeForm;
  changeForm.position = locationalData.pos;
  changeForm.angle = locationalData.rot;
  changeForm.worldOrCellDesc = locationalData.cellOrWorldDesc;
  return changeForm;
}
}

MpObjectReference::MpObjectReference(
  const LocationalData& locationalData_, const FormCallbacks& callbacks_,
  uint32_t baseId_, std::string baseType_,
  std::optional<NiPoint3> primitiveBoundsDiv2)
  : callbacks(new FormCallbacks(callbacks_))
  , baseId(baseId_)
  , baseType(baseType_)
  , ChangeFormGuard(MakeChangeForm(locationalData_), this)
{
  pImpl.reset(new Impl);

  if (primitiveBoundsDiv2)
    SetPrimitive(*primitiveBoundsDiv2);
}

const NiPoint3& MpObjectReference::GetPos() const
{
  return ChangeForm().position;
}

const NiPoint3& MpObjectReference::GetAngle() const
{
  return ChangeForm().angle;
}

const FormDesc& MpObjectReference::GetCellOrWorld() const
{
  return ChangeForm().worldOrCellDesc;
}

const uint32_t& MpObjectReference::GetBaseId() const
{
  return baseId;
}

const std::string& MpObjectReference::GetBaseType() const
{
  return baseType;
}

const Inventory& MpObjectReference::GetInventory() const
{
  return ChangeForm().inv;
}

const bool& MpObjectReference::IsHarvested() const
{
  return ChangeForm().isHarvested;
}

const bool& MpObjectReference::IsOpen() const
{
  return ChangeForm().isOpen;
}

const bool& MpObjectReference::IsDisabled() const
{
  return ChangeForm().isDisabled;
}

const bool& MpObjectReference::IsDeleted() const
{
  return ChangeForm().isDeleted;
}

const uint32_t& MpObjectReference::GetCount() const
{
  return ChangeForm().count;
}

std::chrono::system_clock::duration MpObjectReference::GetRelootTime() const
{
  if (relootTimeOverride) {
    return *relootTimeOverride;
  }

  if (auto time = GetParent()->GetRelootTime(baseType)) {
    return *time;
  }

  if (!std::strcmp(baseType.data(), "FLOR") ||
      !std::strcmp(baseType.data(), "TREE")) {
    return std::chrono::hours(1);
  } else if (!std::strcmp(baseType.data(), "DOOR")) {
    return std::chrono::seconds(3);
  } else if (espm::utils::IsItem(espm::Type{ baseType.data() })) {
    return std::chrono::hours(1);
  } else if (!std::strcmp(baseType.data(), "CONT")) {
    return std::chrono::hours(1);
  }

  return std::chrono::hours(0);
}

bool MpObjectReference::GetAnimationVariableBool(const char* name) const
{
  return pImpl->animGraphHolder &&
    pImpl->animGraphHolder->animationVariablesBool.count(name) > 0;
}

bool MpObjectReference::IsPointInsidePrimitive(const NiPoint3& point) const
{
  if (pImpl->primitive) {
    return Primitive::IsInside(point, pImpl->primitive->polygonProc);
  }
  return false;
}

bool MpObjectReference::HasPrimitive() const
{
  return pImpl->primitive.has_value();
}

FormCallbacks MpObjectReference::GetCallbacks() const
{
  if (!callbacks)
    return FormCallbacks::DoNothing();
  return *callbacks;
}

bool MpObjectReference::HasScript(const char* name) const
{
  return ToGameObject()->HasScript(name);
}

bool MpObjectReference::IsActivationBlocked() const
{
  return activationBlocked;
}

bool MpObjectReference::GetTeleportFlag() const
{
  return pImpl->teleportFlag;
}

void MpObjectReference::VisitProperties(const PropertiesVisitor& visitor,
                                        VisitPropertiesMode mode)
{
  if (IsHarvested()) {
    visitor("isHarvested", "true");
  }

  if (IsOpen()) {
    visitor("isOpen", "true");
  }

  if (auto actor = dynamic_cast<MpActor*>(this); actor && actor->IsDead()) {
    visitor("isDead", "true");
  }

  if (mode == VisitPropertiesMode::All && !GetInventory().IsEmpty()) {
    auto inventoryDump = GetInventory().ToJson().dump();
    visitor("inventory", inventoryDump.data());
  }

  if (IsEspmForm() && IsDisabled()) {
    visitor("disabled", "true");
  }

  if (ChangeForm().lastAnimation.has_value()) {
    std::string raw = *ChangeForm().lastAnimation;
    nlohmann::json j = raw;
    std::string lastAnimationAsJson = j.dump();
    visitor("lastAnimation", lastAnimationAsJson.data());
  }

  if (ChangeForm().setNodeScale.has_value()) {
    // worse performance than building json string manually but proper escaping
    // TODO: consider switching to a faster JSON builder
    nlohmann::json setNodeScaleAsJson;
    for (auto& [key, value] : *ChangeForm().setNodeScale) {
      setNodeScaleAsJson[key] = value;
    }
    visitor("setNodeScale", setNodeScaleAsJson.dump().data());
  }

  if (ChangeForm().setNodeTextureSet.has_value()) {
    // worse performance than building json string manually but proper escaping
    // TODO: consider switching to a faster JSON builder
    nlohmann::json setNodeTextureSetAsJson;
    for (auto& [key, value] : *ChangeForm().setNodeTextureSet) {
      setNodeTextureSetAsJson[key] =
        FormDesc::FromString(value).ToFormId(GetParent()->espmFiles);
    }
    visitor("setNodeTextureSet", setNodeTextureSetAsJson.dump().data());
  }

  if (ChangeForm().displayName.has_value()) {
    const std::string& raw = *ChangeForm().displayName;
    if (raw != PapyrusObjectReference::kOriginalNameExpression) {
      nlohmann::json j = raw;
      std::string displayNameAsJson = j.dump();
      visitor("displayName", displayNameAsJson.data());
    }
  }

  // Property flags (isVisibleByOwner, isVisibleByNeighbor) should be checked
  // by a visitor
  auto& dynamicFields = ChangeForm().dynamicFields.GetAsJson();
  for (auto it = dynamicFields.begin(); it != dynamicFields.end(); ++it) {
    std::string dump = it.value().dump();
    visitor(it.key().data(), dump.data());
  }
}

void MpObjectReference::Activate(MpObjectReference& activationSource,
                                 bool defaultProcessingOnly)
{
  if (spdlog::should_log(spdlog::level::trace)) {
    for (auto& script : ListActivePexInstances()) {
      spdlog::trace("MpObjectReference::Activate {:x} - found script {}",
                    GetFormId(), script->GetSourcePexName());
    }
  }

  if (auto worldState = activationSource.GetParent(); worldState->HasEspm()) {
    CheckInteractionAbility(activationSource);

    // Pillars puzzle Bleak Falls Barrow
    bool workaroundBypassParentsCheck = &activationSource == this;

    // Block if only activation parents can activate this
    auto refrId = GetFormId();
    if (!workaroundBypassParentsCheck && IsEspmForm() &&
        !dynamic_cast<MpActor*>(this)) {
      auto lookupRes = worldState->GetEspm().GetBrowser().LookupById(refrId);
      auto data = espm::GetData<espm::REFR>(refrId, worldState);
      auto it = std::find_if(
        data.activationParents.begin(), data.activationParents.end(),
        [&](const espm::REFR::ActivationParentInfo& info) {
          return lookupRes.ToGlobalId(info.refrId) ==
            activationSource.GetFormId();
        });
      if (it == data.activationParents.end()) {
        if (data.isParentActivationOnly) {
          throw std::runtime_error(
            "Only activation parents can activate this object");
        }
      }
    }
  }

  bool activationBlockedByMpApi = MpApiOnActivate(activationSource);

  if (!activationBlockedByMpApi &&
      (!activationBlocked || defaultProcessingOnly)) {
    ProcessActivate(activationSource);
    ActivateChilds();
  } else {
    spdlog::trace(
      "Activation of form {:#x} has been blocked. Reasons: "
      "blocked by MpApi={}, form is blocked={}, defaultProcessingOnly={}",
      GetFormId(), activationBlockedByMpApi, activationBlocked,
      defaultProcessingOnly);
  }

  if (!defaultProcessingOnly) {
    auto arg = activationSource.ToVarValue();
    SendPapyrusEvent("OnActivate", &arg, 1);
  }
}

void MpObjectReference::Disable()
{
  if (ChangeForm().isDisabled) {
    return;
  }

  EditChangeForm(
    [&](MpChangeFormREFR& changeForm) { changeForm.isDisabled = true; });

  if (!IsEspmForm() || dynamic_cast<MpActor*>(this)) {
    RemoveFromGridAndUnsubscribeAll();
  }
}

void MpObjectReference::Enable()
{
  if (!ChangeForm().isDisabled) {
    return;
  }

  EditChangeForm(
    [&](MpChangeFormREFR& changeForm) { changeForm.isDisabled = false; });

  if (!IsEspmForm() || dynamic_cast<MpActor*>(this)) {
    ForceSubscriptionsUpdate();
  }
}

void MpObjectReference::SetPos(const NiPoint3& newPos)
{
  auto oldGridPos = GetGridPos(ChangeForm().position);
  auto newGridPos = GetGridPos(newPos);

  EditChangeForm(
    [&newPos](MpChangeFormREFR& changeForm) { changeForm.position = newPos; },
    MakeMode(IsLocationSavingNeeded()));

  if (oldGridPos != newGridPos || !everSubscribedOrListened)
    ForceSubscriptionsUpdate();

  if (!IsDisabled()) {
    if (emittersWithPrimitives) {
      if (!primitivesWeAreInside)
        primitivesWeAreInside.reset(new std::set<uint32_t>);

      for (auto& [emitterId, wasInside] : *emittersWithPrimitives) {
        auto& emitter = GetParent()->LookupFormById(emitterId);
        auto emitterRefr =
          std::dynamic_pointer_cast<MpObjectReference>(emitter);
        if (!emitterRefr) {
          GetParent()->logger->error("Emitter not found ({0:x})", emitterId);
          continue;
        }
        bool inside = emitterRefr->IsPointInsidePrimitive(newPos);
        if (wasInside != inside) {
          wasInside = inside;
          auto me = ToVarValue();

          auto wst = GetParent();
          auto id = emitterId;
          auto myId = GetFormId();
          wst->SetTimer(std::chrono::seconds(0))
            .Then([wst, id, inside, me, myId, this](Viet::Void) {
              if (wst->LookupFormById(myId).get() != this) {
                wst->logger->error("Refr pointer expired", id);
                return;
              }

              auto& emitter = wst->LookupFormById(id);
              auto emitterRefr =
                std::dynamic_pointer_cast<MpObjectReference>(emitter);
              if (!emitterRefr) {
                wst->logger->error("Emitter not found in timer ({0:x})", id);
                return;
              }
              emitterRefr->SendPapyrusEvent(
                inside ? "OnTriggerEnter" : "OnTriggerLeave", &me, 1);
            });

          if (inside)
            primitivesWeAreInside->insert(emitterId);
          else
            primitivesWeAreInside->erase(emitterId);
        }
      }
    }

    if (primitivesWeAreInside) {
      auto me = ToVarValue();

      // May be modified inside loop, so copying
      const auto map = *primitivesWeAreInside;

      for (auto emitterId : map) {
        auto& emitter = GetParent()->LookupFormById(emitterId);
        auto emitterRefr =
          std::dynamic_pointer_cast<MpObjectReference>(emitter);
        if (!emitterRefr) {
          GetParent()->logger->error(
            "Emitter not found ({0:x}) when trying to send OnTrigger event",
            emitterId);
          continue;
        }
        emitterRefr->SendPapyrusEvent("OnTrigger", &me, 1);
      }
    }
  }
}

void MpObjectReference::SetAngle(const NiPoint3& newAngle)
{
  EditChangeForm(
    [&](MpChangeFormREFR& changeForm) { changeForm.angle = newAngle; },
    MakeMode(IsLocationSavingNeeded()));
}

void MpObjectReference::SetHarvested(bool harvested)
{
  if (harvested != ChangeForm().isHarvested) {
    EditChangeForm([&](MpChangeFormREFR& changeForm) {
      changeForm.isHarvested = harvested;
    });
    SendPropertyToListeners("isHarvested", harvested);
  }
}

void MpObjectReference::SetOpen(bool open)
{
  if (open != ChangeForm().isOpen) {
    EditChangeForm(
      [&](MpChangeFormREFR& changeForm) { changeForm.isOpen = open; });
    SendPropertyToListeners("isOpen", open);
  }
}

void MpObjectReference::PutItem(MpActor& ac, const Inventory::Entry& e)
{
  CheckInteractionAbility(ac);
  if (this->occupant != &ac) {
    std::stringstream err;
    err << std::hex << "Actor 0x" << ac.GetFormId() << " doesn't occupy ref 0x"
        << GetFormId();
    throw std::runtime_error(err.str());
  }

  if (MpApiOnPutItem(ac, e)) {
    return spdlog::trace("onPutItem - blocked by gamemode");
  }

  spdlog::trace("onPutItem - not blocked by gamemode");
  ac.RemoveItems({ e }, this);
}

void MpObjectReference::TakeItem(MpActor& ac, const Inventory::Entry& e)
{
  CheckInteractionAbility(ac);
  if (this->occupant != &ac) {
    std::stringstream err;
    err << std::hex << "Actor 0x" << ac.GetFormId() << " doesn't occupy ref 0x"
        << GetFormId();
    throw std::runtime_error(err.str());
  }

  if (MpApiOnTakeItem(ac, e)) {
    return spdlog::trace("onTakeItem - blocked by gamemode");
  }

  spdlog::trace("onPutItem - not blocked by gamemode");
  RemoveItems({ e }, &ac);
}

void MpObjectReference::SetRelootTime(
  std::chrono::system_clock::duration newRelootTime)
{
  relootTimeOverride = newRelootTime;
}

void MpObjectReference::SetChanceNoneOverride(uint8_t newChanceNone)
{
  chanceNoneOverride.reset(new uint8_t(newChanceNone));
}

void MpObjectReference::SetCellOrWorld(const FormDesc& newWorldOrCell)
{
  SetCellOrWorldObsolete(newWorldOrCell);
  ForceSubscriptionsUpdate();
}

void MpObjectReference::SetActivationBlocked(bool blocked)
{
  // TODO: Save
  activationBlocked = blocked;
}

void MpObjectReference::ForceSubscriptionsUpdate()
{
  auto worldState = GetParent();
  if (!worldState || IsDisabled()) {
    return;
  }
  InitListenersAndEmitters();

  auto worldOrCell = GetCellOrWorld().ToFormId(worldState->espmFiles);

  auto& gridInfo = worldState->grids[worldOrCell];
  MoveOnGrid(*gridInfo.grid);

  auto& was = *this->listeners;
  auto pos = GetGridPos(GetPos());
  auto& now =
    worldState->GetReferencesAtPosition(worldOrCell, pos.first, pos.second);

  std::vector<MpObjectReference*> toRemove;
  std::set_difference(was.begin(), was.end(), now.begin(), now.end(),
                      std::inserter(toRemove, toRemove.begin()));
  for (auto listener : toRemove) {
    Unsubscribe(this, listener);
    // Unsubscribe from self is NEEDED. See comment below
    if (this != listener)
      Unsubscribe(listener, this);
  }

  std::vector<MpObjectReference*> toAdd;
  std::set_difference(now.begin(), now.end(), was.begin(), was.end(),
                      std::inserter(toAdd, toAdd.begin()));
  for (auto listener : toAdd) {
    Subscribe(this, listener);
    // Note: Self-subscription is OK this check is performed as we don't want
    // to self-subscribe twice! We have already been subscribed to self in
    // the last line of code
    if (this != listener)
      Subscribe(listener, this);
  }

  everSubscribedOrListened = true;
}

void MpObjectReference::SetPrimitive(const NiPoint3& boundsDiv2)
{
  auto vertices = Primitive::GetVertices(GetPos(), GetAngle(), boundsDiv2);
  pImpl->primitive =
    PrimitiveData{ boundsDiv2, Primitive::CreateGeoPolygonProc(vertices) };
}

void MpObjectReference::UpdateHoster(uint32_t newHosterId)
{
  auto hostedMsg = CreatePropertyMessage(this, "isHostedByOther", true);
  auto notHostedMsg = CreatePropertyMessage(this, "isHostedByOther", false);
  for (auto listener : this->GetListeners()) {
    auto listenerAsActor = dynamic_cast<MpActor*>(listener);
    if (listenerAsActor) {
      this->SendPropertyTo(newHosterId != 0 &&
                               newHosterId != listener->GetFormId()
                             ? hostedMsg
                             : notHostedMsg,
                           *listenerAsActor);
    }
  }
}

void MpObjectReference::SetProperty(const std::string& propertyName,
                                    const nlohmann::json& newValue,
                                    bool isVisibleByOwner,
                                    bool isVisibleByNeighbor)
{
  EditChangeForm([&](MpChangeFormREFR& changeForm) {
    changeForm.dynamicFields.Set(propertyName, newValue);
  });
  if (isVisibleByNeighbor) {
    SendPropertyToListeners(propertyName.data(), newValue);
  } else if (isVisibleByOwner) {
    if (auto ac = dynamic_cast<MpActor*>(this)) {
      SendPropertyTo(propertyName.data(), newValue, *ac);
    }
  }
  pImpl->setPropertyCalled = true;
}

void MpObjectReference::SetTeleportFlag(bool value)
{
  pImpl->teleportFlag = value;
}

void MpObjectReference::SetPosAndAngleSilent(const NiPoint3& pos,
                                             const NiPoint3& rot)
{
  EditChangeForm(
    [&](MpChangeFormREFR& changeForm) {
      changeForm.position = pos;
      changeForm.angle = rot;
    },
    Mode::NoRequestSave);
}

void MpObjectReference::Delete()
{
  if (IsEspmForm()) {
    spdlog::warn("MpObjectReference::Delete {:x} - Attempt to delete non-FF "
                 "object, ignoring",
                 GetFormId());
    return;
  }

  EditChangeForm(
    [&](MpChangeFormREFR& changeForm) { changeForm.isDeleted = true; });
  RemoveFromGridAndUnsubscribeAll();
}

void MpObjectReference::SetCount(uint32_t newCount)
{
  EditChangeForm(
    [&](MpChangeFormREFR& changeForm) { changeForm.count = newCount; });
}

void MpObjectReference::SetAnimationVariableBool(const char* name, bool value)
{
  if (!pImpl->animGraphHolder)
    pImpl->animGraphHolder.reset(new AnimGraphHolder);
  if (value)
    pImpl->animGraphHolder->animationVariablesBool.insert(name);
  else
    pImpl->animGraphHolder->animationVariablesBool.erase(name);
}

void MpObjectReference::SetInventory(const Inventory& inv)
{
  EditChangeForm([&](MpChangeFormREFR& changeForm) {
    changeForm.baseContainerAdded = true;
    changeForm.inv = inv;
  });
  SendInventoryUpdate();
}

void MpObjectReference::AddItem(uint32_t baseId, uint32_t count)
{
  EditChangeForm([&](MpChangeFormREFR& changeForm) {
    changeForm.baseContainerAdded = true;
    changeForm.inv.AddItem(baseId, count);
  });
  SendInventoryUpdate();

  //  TODO: No one used it due to incorrect baseItem which should be object,
  //  not id. Needs to be revised. Seems to also be buggy
  // auto baseItem = VarValue(static_cast<int32_t>(baseId));
  // auto itemCount = VarValue(static_cast<int32_t>(count));
  // auto itemReference = VarValue((IGameObject*)nullptr);
  // auto sourceContainer = VarValue((IGameObject*)nullptr);
  // VarValue args[4] = { baseItem, itemCount, itemReference, sourceContainer
  // }; SendPapyrusEvent("OnItemAdded", args, 4);
}

void MpObjectReference::AddItems(const std::vector<Inventory::Entry>& entries)
{
  if (entries.size() > 0) {
    EditChangeForm([&](MpChangeFormREFR& changeForm) {
      changeForm.baseContainerAdded = true;
      changeForm.inv.AddItems(entries);
    });
    SendInventoryUpdate();
  }

  // for (const auto& entri : entries) {
  //   auto baseItem = VarValue(static_cast<int32_t>(entri.baseId));
  //   auto itemCount = VarValue(static_cast<int32_t>(entri.count));
  //   auto itemReference = VarValue((IGameObject*)nullptr);
  //   auto sourceContainer = VarValue((IGameObject*)nullptr);
  //   VarValue args[4] = { baseItem, itemCount, itemReference,
  //   sourceContainer
  //   }; SendPapyrusEvent("OnItemAdded", args, 4);
  // }
}

void MpObjectReference::RemoveItem(uint32_t baseId, uint32_t count,
                                   MpObjectReference* target)
{
  RemoveItems({ { baseId, count } }, target);
}

void MpObjectReference::RemoveItems(
  const std::vector<Inventory::Entry>& entries, MpObjectReference* target)
{
  EditChangeForm([&](MpChangeFormREFR& changeForm) {
    changeForm.inv.RemoveItems(entries);
  });

  if (target)
    target->AddItems(entries);

  SendInventoryUpdate();

  if (GetBaseType() == "CONT") {
    if (GetInventory().IsEmpty()) {
      spdlog::info("MpObjectReference::RemoveItems - {:x} requesting reloot",
                   this->GetFormId());
      RequestReloot();
    }
  }
}

void MpObjectReference::RemoveAllItems(MpObjectReference* target)
{
  auto prevInv = GetInventory();
  RemoveItems(prevInv.entries, target);
}

void MpObjectReference::RelootContainer()
{
  EditChangeForm(
    [&](MpChangeFormREFR& changeForm) {
      changeForm.baseContainerAdded = false;
    },
    Mode::NoRequestSave);
  EnsureBaseContainerAdded(*GetParent()->espm);
}

void MpObjectReference::RegisterProfileId(int32_t profileId)
{
  auto worldState = GetParent();
  if (!worldState) {
    throw std::runtime_error("Not attached to WorldState");
  }

  if (profileId < 0) {
    throw std::runtime_error(
      "Negative profileId passed to RegisterProfileId, should be >= 0");
  }

  auto currentProfileId = ChangeForm().profileId;
  auto formId = GetFormId();

  if (currentProfileId > 0) {
    worldState->actorIdByProfileId[currentProfileId].erase(formId);
  }

  EditChangeForm(
    [&](MpChangeFormREFR& changeForm) { changeForm.profileId = profileId; });

  if (profileId > 0) {
    worldState->actorIdByProfileId[profileId].insert(formId);
  }
}

void MpObjectReference::RegisterPrivateIndexedProperty(
  const std::string& propertyName, const std::string& propertyValueStringified)
{
  auto worldState = GetParent();
  if (!worldState) {
    throw std::runtime_error("Not attached to WorldState");
  }

  bool (*isNull)(const std::string&) = [](const std::string& s) {
    return s == "null" || s == "undefined" || s == "" || s == "''" ||
      s == "\"\"";
  };

  auto currentValueStringified =
    ChangeForm().dynamicFields.Get(propertyName).dump();
  auto formId = GetFormId();
  if (!isNull(currentValueStringified)) {
    auto key = worldState->MakePrivateIndexedPropertyMapKey(
      propertyName, currentValueStringified);
    worldState->actorIdByPrivateIndexedProperty[key].erase(formId);
    spdlog::trace("MpObjectReference::RegisterPrivateIndexedProperty {:x} - "
                  "unregister {}",
                  formId, key);
  }

  EditChangeForm([&](MpChangeFormREFR& changeForm) {
    auto propertyValue = nlohmann::json::parse(propertyValueStringified);
    changeForm.dynamicFields.Set(propertyName, propertyValue);
  });

  if (!isNull(propertyValueStringified)) {
    auto key = worldState->MakePrivateIndexedPropertyMapKey(
      propertyName, propertyValueStringified);
    worldState->actorIdByPrivateIndexedProperty[key].insert(formId);
    spdlog::trace(
      "MpObjectReference::RegisterPrivateIndexedProperty {:x} - register {}",
      formId, key);
  }
}

void MpObjectReference::Subscribe(MpObjectReference* emitter,
                                  MpObjectReference* listener)
{
  auto actorEmitter = dynamic_cast<MpActor*>(emitter);
  auto actorListener = dynamic_cast<MpActor*>(listener);
  if (!actorEmitter && !actorListener) {
    return;
  }

  // I don't know how often Subscrbe is called but I suppose
  // it is to be invoked quite frequently. In this case, each
  // time if below is performed we are obtaining a copy of
  // MpChangeForm which can be large. See what it consists of.
  if (!emitter->pImpl->onInitEventSent &&
      listener->GetChangeForm().profileId != -1) {
    emitter->pImpl->onInitEventSent = true;
    emitter->SendPapyrusEvent("OnInit");
    emitter->SendPapyrusEvent("OnCellLoad");
    emitter->SendPapyrusEvent("OnLoad");
  }

  const bool hasPrimitive = emitter->HasPrimitive();

  emitter->InitListenersAndEmitters();
  listener->InitListenersAndEmitters();
  emitter->listeners->insert(listener);
  if (actorListener) {
    emitter->actorListeners.insert(actorListener);
  }
  listener->emitters->insert(emitter);
  if (!hasPrimitive) {
    emitter->callbacks->subscribe(emitter, listener);
  }

  if (hasPrimitive) {
    if (!listener->emittersWithPrimitives) {
      listener->emittersWithPrimitives.reset(new std::map<uint32_t, bool>);
    }
    listener->emittersWithPrimitives->insert({ emitter->GetFormId(), false });
  }
}

void MpObjectReference::Unsubscribe(MpObjectReference* emitter,
                                    MpObjectReference* listener)
{
  auto actorEmitter = dynamic_cast<MpActor*>(emitter);
  auto actorListener = dynamic_cast<MpActor*>(listener);
  bool bothNonActors = !actorEmitter && !actorListener;
  if (bothNonActors) {
    return;
  }

  const bool hasPrimitive = emitter->HasPrimitive();

  if (!hasPrimitive) {
    emitter->callbacks->unsubscribe(emitter, listener);
  }
  emitter->listeners->erase(listener);
  if (actorListener) {
    emitter->actorListeners.erase(actorListener);
  }
  listener->emitters->erase(emitter);

  if (listener->emittersWithPrimitives && hasPrimitive) {
    listener->emittersWithPrimitives->erase(emitter->GetFormId());
  }
}

void MpObjectReference::SetLastAnimation(const std::string& lastAnimation)
{
  EditChangeForm([&](MpChangeForm& changeForm) {
    changeForm.lastAnimation = lastAnimation;
  });
}

void MpObjectReference::SetNodeTextureSet(const std::string& node,
                                          const espm::LookupResult& textureSet,
                                          bool firstPerson)
{
  // This only changes var in database, SpSnippet is being sent in
  // PapyrusNetImmerse.cpp
  EditChangeForm([&](MpChangeForm& changeForm) {
    if (changeForm.setNodeTextureSet == std::nullopt) {
      changeForm.setNodeTextureSet = std::map<std::string, std::string>();
    }

    uint32_t textureSetId = textureSet.ToGlobalId(textureSet.rec->GetId());

    FormDesc textureSetFormDesc =
      FormDesc::FromFormId(textureSetId, GetParent()->espmFiles);

    changeForm.setNodeTextureSet->insert_or_assign(
      node, textureSetFormDesc.ToString());
  });
}

void MpObjectReference::SetNodeScale(const std::string& node, float scale,
                                     bool firstPerson)
{
  // This only changes var in database, SpSnippet is being sent in
  // PapyrusNetImmerse.cpp
  EditChangeForm([&](MpChangeForm& changeForm) {
    if (changeForm.setNodeScale == std::nullopt) {
      changeForm.setNodeScale = std::map<std::string, float>();
    }
    changeForm.setNodeScale->insert_or_assign(node, scale);
  });
}

void MpObjectReference::SetDisplayName(
  const std::optional<std::string>& newName)
{
  EditChangeForm(
    [&](MpChangeForm& changeForm) { changeForm.displayName = newName; });
}

const std::set<MpObjectReference*>& MpObjectReference::GetListeners() const
{
  static const std::set<MpObjectReference*> kEmptyListeners;
  return listeners ? *listeners : kEmptyListeners;
}

const std::set<MpActor*>& MpObjectReference::GetActorListeners() const noexcept
{
  return actorListeners;
}

const std::set<MpObjectReference*>& MpObjectReference::GetEmitters() const
{
  static const std::set<MpObjectReference*> kEmptyEmitters;
  return emitters ? *emitters : kEmptyEmitters;
}

void MpObjectReference::RequestReloot(
  std::optional<std::chrono::system_clock::duration> time)
{
  if (!IsEspmForm()) {
    return;
  }

  if (GetParent()->IsRelootForbidden(baseType)) {
    return;
  }

  if (!time)
    time = GetRelootTime();

  if (!ChangeForm().nextRelootDatetime) {
    EditChangeForm([&](MpChangeFormREFR& changeForm) {
      changeForm.nextRelootDatetime = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now() + GetRelootTime());
    });

    GetParent()->RequestReloot(*this, *time);
  }
}

void MpObjectReference::DoReloot()
{
  if (ChangeForm().nextRelootDatetime) {
    EditChangeForm([&](MpChangeFormREFR& changeForm) {
      changeForm.nextRelootDatetime = 0;
    });
    SetOpen(false);
    SetHarvested(false);
    RelootContainer();
  }
}

std::shared_ptr<std::chrono::time_point<std::chrono::system_clock>>
MpObjectReference::GetNextRelootMoment() const
{
  std::shared_ptr<std::chrono::time_point<std::chrono::system_clock>> res;
  if (ChangeForm().nextRelootDatetime) {
    res.reset(new std::chrono::time_point<std::chrono::system_clock>(
      std::chrono::system_clock::from_time_t(
        ChangeForm().nextRelootDatetime)));
  }
  return res;
}

MpChangeForm MpObjectReference::GetChangeForm() const
{
  MpChangeForm res = ChangeForm();

  if (GetParent() && !GetParent()->espmFiles.empty()) {
    res.formDesc = FormDesc::FromFormId(GetFormId(), GetParent()->espmFiles);
    res.baseDesc = FormDesc::FromFormId(GetBaseId(), GetParent()->espmFiles);
  } else {
    res.formDesc = res.baseDesc = FormDesc(GetFormId(), "");
  }

  return res;
}

void MpObjectReference::ApplyChangeForm(const MpChangeForm& changeForm)
{
  if (pImpl->setPropertyCalled) {
    GetParent()->logger->critical("ApplyChangeForm called after SetProperty");
    std::terminate();
  }

  blockSaving = true;
  Viet::ScopedTask<MpObjectReference> unblockTask(
    [](MpObjectReference& self) { self.blockSaving = false; }, *this);

  const auto currentBaseId = GetBaseId();
  const auto newBaseId = changeForm.baseDesc.ToFormId(GetParent()->espmFiles);
  if (currentBaseId != newBaseId) {
    spdlog::error("Anomaly, baseId should never change ({:x} => {:x})",
                  currentBaseId, newBaseId);
  }

  if (ChangeForm().formDesc != changeForm.formDesc) {
    throw std::runtime_error("Expected formDesc to be " +
                             ChangeForm().formDesc.ToString() +
                             ", but found " + changeForm.formDesc.ToString());
  }

  if (changeForm.profileId >= 0) {
    RegisterProfileId(changeForm.profileId);
  }

  changeForm.dynamicFields.ForEach(
    [&](const std::string& propertyName, const nlohmann::json& value) {
      static const std::string kPrefix = GetPropertyPrefixPrivateIndexed();
      bool startsWith = propertyName.compare(0, kPrefix.size(), kPrefix) == 0;
      if (startsWith) {
        RegisterPrivateIndexedProperty(propertyName, value.dump());
      }
    });

  // See https://github.com/skyrim-multiplayer/issue-tracker/issues/42
  EditChangeForm(
    [&](MpChangeFormREFR& f) {
      f = changeForm;

      // Fix: RequestReloot doesn't work with non-zero 'nextRelootDatetime'
      f.nextRelootDatetime = 0;
    },
    Mode::NoRequestSave);
  if (changeForm.nextRelootDatetime) {
    auto tp =
      std::chrono::system_clock::from_time_t(changeForm.nextRelootDatetime);

    // Fix: Handle situations when the server is stopped at the 'tp' moment
    if (tp < std::chrono::system_clock::now()) {
      tp = std::chrono::system_clock::now() + std::chrono::milliseconds(1);
    }

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      tp - std::chrono::system_clock::now());
    RequestReloot(ms);
  }

  // Perform all required grid operations
  // Mirrors MpActor impl
  // TODO: get rid of dynamic_cast
  if (!dynamic_cast<MpActor*>(this)) {
    changeForm.isDisabled ? Disable() : Enable();
    SetCellOrWorldObsolete(changeForm.worldOrCellDesc);
    SetPos(changeForm.position);
  }
}

const DynamicFields& MpObjectReference::GetDynamicFields() const
{
  return ChangeForm().dynamicFields;
}

void MpObjectReference::SetCellOrWorldObsolete(const FormDesc& newWorldOrCell)
{
  auto worldState = GetParent();
  if (!worldState) {
    return;
  }

  auto worldOrCell =
    ChangeForm().worldOrCellDesc.ToFormId(worldState->espmFiles);

  everSubscribedOrListened = false;
  auto gridIterator = worldState->grids.find(worldOrCell);
  if (gridIterator != worldState->grids.end()) {
    gridIterator->second.grid->Forget(this);
  }

  EditChangeForm([&](MpChangeFormREFR& changeForm) {
    changeForm.worldOrCellDesc = newWorldOrCell;
  });
}

void MpObjectReference::VisitNeighbours(const Visitor& visitor)
{
  if (IsDisabled()) {
    return;
  }

  auto worldState = GetParent();
  if (!worldState) {
    return;
  }

  auto worldOrCell =
    ChangeForm().worldOrCellDesc.ToFormId(worldState->espmFiles);

  auto gridIterator = worldState->grids.find(worldOrCell);
  if (gridIterator == worldState->grids.end()) {
    return;
  }

  auto& grid = gridIterator->second;
  auto pos = GetGridPos(GetPos());
  auto& neighbours =
    worldState->GetReferencesAtPosition(worldOrCell, pos.first, pos.second);
  for (auto neighbour : neighbours) {
    visitor(neighbour);
  }
}

void MpObjectReference::SendPapyrusEvent(const char* eventName,
                                         const VarValue* arguments,
                                         size_t argumentsCount)
{
  if (!pImpl->scriptsInited) {
    InitScripts();
    pImpl->scriptsInited = true;
  }
  return MpForm::SendPapyrusEvent(eventName, arguments, argumentsCount);
}

void MpObjectReference::Init(WorldState* parent, uint32_t formId,
                             bool hasChangeForm)
{
  MpForm::Init(parent, formId, hasChangeForm);

  // We should queue created form for saving as soon as it is initialized
  const auto mode = (!hasChangeForm && !IsEspmForm()) ? Mode::RequestSave
                                                      : Mode::NoRequestSave;

  EditChangeForm(
    [&](MpChangeFormREFR& changeForm) {
      changeForm.formDesc =
        FormDesc::FromFormId(formId, GetParent()->espmFiles);
    },
    mode);

  auto refrId = GetFormId();
  if (parent->HasEspm() && IsEspmForm() && !dynamic_cast<MpActor*>(this)) {
    auto lookupRes = parent->GetEspm().GetBrowser().LookupById(refrId);
    auto data = espm::GetData<espm::REFR>(refrId, parent);
    for (auto& info : data.activationParents) {
      auto activationParent = lookupRes.ToGlobalId(info.refrId);

      // Using WorldState for that, because we don't want search (potentially
      // load) other references during OnInit
      parent->activationChildsByActivationParent[activationParent].insert(
        { refrId, info.delay });
    }
  }
}

bool MpObjectReference::IsLocationSavingNeeded() const
{
  auto last = GetLastSaveRequestMoment();
  return !last ||
    std::chrono::system_clock::now() - *last > std::chrono::seconds(30);
}

void MpObjectReference::ProcessActivate(MpObjectReference& activationSource)
{
  auto actorActivator = dynamic_cast<MpActor*>(&activationSource);

  auto worldState = GetParent();
  auto& loader = GetParent()->GetEspm();
  auto& compressedFieldsCache = GetParent()->GetEspmCache();

  auto base = loader.GetBrowser().LookupById(GetBaseId());
  if (!base.rec || !GetBaseId()) {
    std::stringstream ss;
    ss << std::hex << GetFormId() << " doesn't have base form";
    throw std::runtime_error(ss.str());
  }

  auto t = base.rec->GetType();

  bool pickable = espm::utils::Is<espm::TREE>(t) ||
    espm::utils::Is<espm::FLOR>(t) || espm::utils::IsItem(t);
  if (pickable && !IsHarvested()) {
    auto mapping = loader.GetBrowser().GetCombMapping(base.fileIdx);
    uint32_t resultItem = 0;
    if (espm::utils::Is<espm::TREE>(t)) {
      auto data =
        espm::Convert<espm::TREE>(base.rec)->GetData(compressedFieldsCache);
      resultItem = espm::utils::GetMappedId(data.resultItem, *mapping);
    }

    if (espm::utils::Is<espm::FLOR>(t)) {
      auto data =
        espm::Convert<espm::FLOR>(base.rec)->GetData(compressedFieldsCache);
      resultItem = espm::utils::GetMappedId(data.resultItem, *mapping);
    }

    if (espm::utils::Is<espm::LIGH>(t)) {
      auto res =
        espm::Convert<espm::LIGH>(base.rec)->GetData(compressedFieldsCache);
      bool isTorch = res.data.flags & espm::LIGH::Flags::CanBeCarried;
      if (!isTorch) {
        return;
      }
      resultItem = espm::utils::GetMappedId(base.rec->GetId(), *mapping);
    }

    if (resultItem == 0) {
      resultItem = espm::utils::GetMappedId(base.rec->GetId(), *mapping);
    }

    auto resultItemLookupRes = loader.GetBrowser().LookupById(resultItem);
    auto leveledItem = espm::Convert<espm::LVLI>(resultItemLookupRes.rec);
    if (leveledItem) {
      const auto kCountMult = 1;
      auto map = LeveledListUtils::EvaluateListRecurse(
        loader.GetBrowser(), resultItemLookupRes, kCountMult,
        kPlayerCharacterLevel, chanceNoneOverride.get());
      for (auto& p : map) {
        activationSource.AddItem(p.first, p.second);
      }
    } else {
      auto refrRecord = espm::Convert<espm::REFR>(
        loader.GetBrowser().LookupById(GetFormId()).rec);

      uint32_t countRecord =
        refrRecord ? refrRecord->GetData(compressedFieldsCache).count : 1;

      uint32_t countChangeForm = ChangeForm().count;

      constexpr uint32_t kCountDefault = 1;

      uint32_t resultingCount =
        std::max(kCountDefault, std::max(countRecord, countChangeForm));

      activationSource.AddItem(resultItem, resultingCount);
    }
    SetHarvested(true);
    RequestReloot();

    if (espm::utils::IsItem(t) && !IsEspmForm()) {
      spdlog::info("MpObjectReference::ProcessActivate - Deleting 0xff item");
      Delete();
    }
  } else if (t == espm::DOOR::kType) {
    auto lookupRes = loader.GetBrowser().LookupById(GetFormId());
    auto refrRecord = espm::Convert<espm::REFR>(lookupRes.rec);
    auto teleport = refrRecord->GetData(compressedFieldsCache).teleport;
    if (teleport) {
      if (!IsOpen()) {
        SetOpen(true);
        RequestReloot();
      }

      auto destinationId = lookupRes.ToGlobalId(teleport->destinationDoor);
      auto destination = loader.GetBrowser().LookupById(destinationId);
      auto destinationRecord = espm::Convert<espm::REFR>(destination.rec);
      if (!destinationRecord) {
        throw std::runtime_error(
          "No destination found for this teleport door");
      }

      auto teleportWorldOrCell = destination.ToGlobalId(
        GetWorldOrCell(loader.GetBrowser(), destinationRecord));

      static const auto kPi = std::acos(-1.f);
      const auto& pos = teleport->pos;
      const float rot[] = { teleport->rotRadians[0] / kPi * 180,
                            teleport->rotRadians[1] / kPi * 180,
                            teleport->rotRadians[2] / kPi * 180 };

      TeleportMessage msg;
      msg.idx = activationSource.GetIdx();
      std::copy(std::begin(pos), std::end(pos), msg.pos.begin());
      std::copy(std::begin(rot), std::end(rot), msg.rot.begin());
      msg.worldOrCell = teleportWorldOrCell;

      if (actorActivator) {
        actorActivator->SendToUser(msg, true);
      }

      activationSource.SetCellOrWorldObsolete(
        FormDesc::FromFormId(teleportWorldOrCell, worldState->espmFiles));
      activationSource.SetPos({ pos[0], pos[1], pos[2] });
      activationSource.SetAngle({ rot[0], rot[1], rot[2] });

    } else {
      SetOpen(!IsOpen());
    }
  } else if (t == espm::CONT::kType && actorActivator) {
    EnsureBaseContainerAdded(loader);

    auto occupantPos = this->occupant ? this->occupant->GetPos() : NiPoint3();
    auto occupantCellOrWorld =
      this->occupant ? this->occupant->GetCellOrWorld() : FormDesc();

    constexpr float kOccupationReach = 512.f;
    auto distanceToOccupant = (occupantPos - GetPos()).Length();

    if (!this->occupant || this->occupant->IsDisabled() ||
        distanceToOccupant > kOccupationReach ||
        occupantCellOrWorld != GetCellOrWorld()) {
      if (this->occupant) {
        this->occupant->RemoveEventSink(this->occupantDestroySink);
      }
      SetOpen(true);
      SendPropertyTo("inventory", GetInventory().ToJson(), *actorActivator);
      activationSource.SendOpenContainer(GetFormId());

      this->occupant = actorActivator;

      this->occupantDestroySink.reset(
        new OccupantDestroyEventSink(*GetParent(), this));
      this->occupant->AddEventSink(occupantDestroySink);
    } else if (this->occupant == &activationSource) {
      SetOpen(false);
      this->occupant->RemoveEventSink(this->occupantDestroySink);
      this->occupant = nullptr;
    }
  }
}

void MpObjectReference::ActivateChilds()
{
  auto worldState = GetParent();
  if (!worldState) {
    return;
  }

  auto myFormId = GetFormId();

  for (auto& pair : worldState->activationChildsByActivationParent[myFormId]) {
    auto childRefrId = pair.first;
    auto delay = pair.second;

    auto delayMs = Viet::TimeUtils::To<std::chrono::milliseconds>(delay);
    worldState->SetTimer(delayMs).Then([worldState, childRefrId,
                                        myFormId](Viet::Void) {
      auto childRefr = std::dynamic_pointer_cast<MpObjectReference>(
        worldState->LookupFormById(childRefrId));
      if (!childRefr) {
        spdlog::warn("MpObjectReference::ActivateChilds {:x} - Bad/missing "
                     "activation child {:x}",
                     myFormId, childRefrId);
        return;
      }

      // Not sure about activationSource and defaultProcessingOnly in this
      // case I'll try to keep vanilla scripts working
      childRefr->Activate(worldState->GetFormAt<MpObjectReference>(myFormId));
    });
  }
}

bool MpObjectReference::MpApiOnActivate(MpObjectReference& caster)
{
  simdjson::dom::parser parser;

  std::string s = "[" + std::to_string(caster.GetFormId()) + " ]";
  auto args = parser.parse(s).value();

  bool activationBlocked = false;

  if (auto wst = GetParent()) {
    const auto id = GetFormId();
    for (auto& listener : wst->listeners) {
      if (listener->OnMpApiEvent("onActivate", args, id) == false) {
        activationBlocked = true;
      }
    }
  }

  return activationBlocked;
}

void MpObjectReference::RemoveFromGridAndUnsubscribeAll()
{
  auto worldOrCell = GetCellOrWorld().ToFormId(GetParent()->espmFiles);
  auto gridIterator = GetParent()->grids.find(worldOrCell);
  if (gridIterator != GetParent()->grids.end()) {
    gridIterator->second.grid->Forget(this);
  }

  auto listenersCopy = GetListeners();
  for (auto listener : listenersCopy) {
    Unsubscribe(this, listener);
  }

  everSubscribedOrListened = false;
}

void MpObjectReference::UnsubscribeFromAll()
{
  auto emittersCopy = GetEmitters();
  for (auto emitter : emittersCopy)
    Unsubscribe(emitter, this);
}

void MpObjectReference::InitScripts()
{
  auto baseId = GetBaseId();
  if (!baseId || !GetParent()->espm) {
    return;
  }

  auto scriptStorage = GetParent()->GetScriptStorage();
  if (!scriptStorage) {
    return;
  }

  auto& compressedFieldsCache = GetParent()->GetEspmCache();

  std::vector<std::string> scriptNames;

  auto& br = GetParent()->espm->GetBrowser();
  auto base = br.LookupById(baseId);
  auto refr = br.LookupById(GetFormId());
  for (auto record : { base.rec, refr.rec }) {
    if (!record) {
      continue;
    }

    std::optional<espm::ScriptData> scriptData;

    if (record == base.rec && record->GetType() == "NPC_") {
      auto baseId = base.ToGlobalId(base.rec->GetId());
      if (auto actor = dynamic_cast<MpActor*>(this)) {
        auto& templateChain = actor->GetTemplateChain();
        scriptData = EvaluateTemplate<espm::NPC_::UseScript>(
          GetParent(), baseId, templateChain,
          [&compressedFieldsCache](const auto& npcLookupRes,
                                   const auto& npcData) {
            espm::ScriptData scriptData;
            npcLookupRes.rec->GetScriptData(&scriptData,
                                            compressedFieldsCache);
            return scriptData;
          });
      }
    }

    if (!scriptData) {
      scriptData = espm::ScriptData();
      record->GetScriptData(&*scriptData, compressedFieldsCache);
    }

    auto& scriptsInStorage =
      GetParent()->GetScriptStorage()->ListScripts(false);
    for (auto& script : scriptData->scripts) {
      if (scriptsInStorage.count(
            { script.scriptName.begin(), script.scriptName.end() })) {

        if (std::count(scriptNames.begin(), scriptNames.end(),
                       script.scriptName) == 0) {
          scriptNames.push_back(script.scriptName);
        }
      } else if (auto wst = GetParent())
        wst->logger->warn("Script '{}' not found in the script storage",
                          script.scriptName);
    }
  }

  // A hardcoded hack to remove all scripts except SweetPie scripts from
  // exterior objects
  if (GetParent() && GetParent()->disableVanillaScriptsInExterior &&
      GetFormId() < 0x05000000) {
    auto cellOrWorld = GetCellOrWorld().ToFormId(GetParent()->espmFiles);
    auto lookupRes =
      GetParent()->GetEspm().GetBrowser().LookupById(cellOrWorld);
    if (lookupRes.rec && lookupRes.rec->GetType() == "WRLD") {
      spdlog::info("Skipping non-Sweet scripts for exterior form {:x}");
      scriptNames.erase(std::remove_if(scriptNames.begin(), scriptNames.end(),
                                       [](const std::string& val) {
                                         auto kPrefix = "Sweet";
                                         bool startsWith = val.size() >= 5 &&
                                           !memcmp(kPrefix, val.data(), 5);
                                         return !startsWith;
                                       }),
                        scriptNames.end());
    }
  }

  if (!scriptNames.empty()) {
    pImpl->scriptState = std::make_unique<ScriptState>();

    std::vector<VirtualMachine::ScriptInfo> scriptInfo;
    for (auto& scriptName : scriptNames) {
      auto scriptVariablesHolder = std::make_shared<ScriptVariablesHolder>(
        scriptName, base, refr, base.parent, &compressedFieldsCache,
        GetParent());
      scriptInfo.push_back({ scriptName, std::move(scriptVariablesHolder) });
    }

    GetParent()->GetPapyrusVm().AddObject(ToGameObject(), scriptInfo);
  }
}

void MpObjectReference::MoveOnGrid(GridImpl<MpObjectReference*>& grid)
{
  auto newGridPos = GetGridPos(GetPos());
  grid.Move(this, newGridPos.first, newGridPos.second);
}

void MpObjectReference::InitListenersAndEmitters()
{
  if (!listeners) {
    listeners.reset(new std::set<MpObjectReference*>);
    emitters.reset(new std::set<MpObjectReference*>);
    actorListeners.clear();
  }
}

void MpObjectReference::SendInventoryUpdate()
{
  constexpr int kChannelSetInventory = 0;
  auto actor = dynamic_cast<MpActor*>(this);
  if (actor) {
    std::string msg;
    msg += Networking::MinPacketId;
    msg += nlohmann::json{
      { "inventory", actor->GetInventory().ToJson() },
      { "type", "setInventory" }
    }.dump();
    actor->SendToUserDeferred(msg.data(), msg.size(), true,
                              kChannelSetInventory, true);
  }
}

void MpObjectReference::SendOpenContainer(uint32_t targetId)
{
  auto actor = dynamic_cast<MpActor*>(this);
  if (actor) {
    OpenContainerMessage msg;
    msg.target = targetId;
    actor->SendToUser(msg, true);
  }
}

std::vector<espm::CONT::ContainerObject> GetOutfitObjects(
  WorldState* worldState, const std::vector<FormDesc>& templateChain,
  const espm::LookupResult& lookupRes)
{
  auto& compressedFieldsCache = worldState->GetEspmCache();

  std::vector<espm::CONT::ContainerObject> res;

  if (auto baseNpc = espm::Convert<espm::NPC_>(lookupRes.rec)) {
    auto baseId = lookupRes.ToGlobalId(lookupRes.rec->GetId());
    auto outfitId = EvaluateTemplate<espm::NPC_::UseInventory>(
      worldState, baseId, templateChain,
      [](const auto& npcLookupRes, const auto& npcData) {
        return npcLookupRes.ToGlobalId(npcData.defaultOutfitId);
      });

    auto outfitLookupRes =
      worldState->GetEspm().GetBrowser().LookupById(outfitId);
    auto outfit = espm::Convert<espm::OTFT>(outfitLookupRes.rec);
    auto outfitData =
      outfit ? outfit->GetData(compressedFieldsCache) : espm::OTFT::Data();

    for (uint32_t i = 0; i != outfitData.count; ++i) {
      auto outfitElementId = outfitLookupRes.ToGlobalId(outfitData.formIds[i]);
      res.push_back({ outfitElementId, 1 });
    }
  }
  return res;
}

std::vector<espm::CONT::ContainerObject> GetInventoryObjects(
  WorldState* worldState, const std::vector<FormDesc>& templateChain,
  const espm::LookupResult& lookupRes)
{
  auto& compressedFieldsCache = worldState->GetEspmCache();

  auto baseContainer = espm::Convert<espm::CONT>(lookupRes.rec);
  if (baseContainer) {
    return baseContainer->GetData(compressedFieldsCache).objects;
  }

  auto baseNpc = espm::Convert<espm::NPC_>(lookupRes.rec);
  if (baseNpc) {
    auto baseId = lookupRes.ToGlobalId(lookupRes.rec->GetId());
    return EvaluateTemplate<espm::NPC_::UseInventory>(
      worldState, baseId, templateChain,
      [](const auto&, const auto& npcData) { return npcData.objects; });
  }

  return {};
}

void MpObjectReference::AddContainerObject(
  const espm::CONT::ContainerObject& entry,
  std::map<uint32_t, uint32_t>* itemsToAdd)
{
  auto& espm = GetParent()->GetEspm();
  auto formLookupRes = espm.GetBrowser().LookupById(entry.formId);
  auto leveledItem = espm::Convert<espm::LVLI>(formLookupRes.rec);
  if (leveledItem) {
    constexpr uint32_t kCountMult = 1;
    auto map = LeveledListUtils::EvaluateListRecurse(
      espm.GetBrowser(), formLookupRes, kCountMult, kPlayerCharacterLevel,
      chanceNoneOverride.get());
    for (auto& p : map) {
      (*itemsToAdd)[p.first] += p.second;
    }
  } else {
    (*itemsToAdd)[entry.formId] += entry.count;
  }
}

void MpObjectReference::EnsureBaseContainerAdded(espm::Loader& espm)
{
  if (ChangeForm().baseContainerAdded) {
    return;
  }

  auto worldState = GetParent();
  if (!worldState) {
    return;
  }

  auto actor = dynamic_cast<MpActor*>(this);
  const std::vector<FormDesc> kEmptyTemplateChain;
  const std::vector<FormDesc>& templateChain =
    actor ? actor->GetTemplateChain() : kEmptyTemplateChain;

  auto lookupRes = espm.GetBrowser().LookupById(GetBaseId());

  std::map<uint32_t, uint32_t> itemsToAdd, itemsToEquip;

  auto inventoryObjects =
    GetInventoryObjects(GetParent(), templateChain, lookupRes);
  for (auto& entry : inventoryObjects) {
    AddContainerObject(entry, &itemsToAdd);
  }

  auto outfitObjects = GetOutfitObjects(GetParent(), templateChain, lookupRes);
  for (auto& entry : outfitObjects) {
    AddContainerObject(entry, &itemsToAdd);
    AddContainerObject(entry, &itemsToEquip);
  }
  if (actor) {
    Equipment eq;
    for (auto& p : itemsToEquip) {
      Inventory::Entry e;
      e.baseId = p.first;
      e.count = p.second;
      e.extra.worn = Inventory::Worn::Right;
      eq.inv.AddItems({ e });
    }
    actor->SetEquipment(eq.ToJson().dump());
  }

  std::vector<Inventory::Entry> entries;
  for (auto& p : itemsToAdd) {
    entries.push_back({ p.first, p.second });
  }
  AddItems(entries);

  if (!ChangeForm().baseContainerAdded) {
    EditChangeForm([&](MpChangeFormREFR& changeForm) {
      changeForm.baseContainerAdded = true;
    });
  }
}

void MpObjectReference::CheckInteractionAbility(MpObjectReference& refr)
{
  auto& loader = GetParent()->GetEspm();
  auto& compressedFieldsCache = GetParent()->GetEspmCache();

  auto casterWorldId = refr.GetCellOrWorld().ToFormId(GetParent()->espmFiles);
  auto targetWorldId = GetCellOrWorld().ToFormId(GetParent()->espmFiles);

  auto casterWorld = loader.GetBrowser().LookupById(casterWorldId).rec;
  auto targetWorld = loader.GetBrowser().LookupById(targetWorldId).rec;

  if (targetWorld != casterWorld) {
    const char* casterWorldName =
      casterWorld ? casterWorld->GetEditorId(compressedFieldsCache) : "<null>";

    const char* targetWorldName =
      targetWorld ? targetWorld->GetEditorId(compressedFieldsCache) : "<null>";

    throw std::runtime_error(fmt::format(
      "WorldSpace doesn't match: caster is in {} ({:#x}), target is in "
      "{} ({:#x})",
      casterWorldName, casterWorldId, targetWorldName, targetWorldId));
  }
}

void MpObjectReference::SendPropertyToListeners(const char* name,
                                                const nlohmann::json& value)
{
  auto msg = CreatePropertyMessage(this, name, value);
  for (auto listener : GetListeners()) {
    auto listenerAsActor = dynamic_cast<MpActor*>(listener);
    if (listenerAsActor) {
      listenerAsActor->SendToUser(msg, true);
    }
  }
}

void MpObjectReference::SendPropertyTo(const char* name,
                                       const nlohmann::json& value,
                                       MpActor& target)
{
  auto msg = CreatePropertyMessage(this, name, value);
  SendPropertyTo(msg, target);
}

void MpObjectReference::SendPropertyTo(const IMessageBase& preparedPropMsg,
                                       MpActor& target)
{
  target.SendToUser(preparedPropMsg, true);
}

void MpObjectReference::BeforeDestroy()
{
  if (this->occupant && this->occupantDestroySink) {
    this->occupant->RemoveEventSink(this->occupantDestroySink);
  }

  // Move far far away calling OnTriggerExit, unsubscribing, etc
  SetPos({ -1'000'000'000, 0, 0 });

  MpForm::BeforeDestroy();

  RemoveFromGridAndUnsubscribeAll();
}

bool MpObjectReference::MpApiOnPutItem(MpActor& source,
                                       const Inventory::Entry& entry)
{
  simdjson::dom::parser parser;
  std::string rawArgs = "[" + std::to_string(source.GetFormId()) + "," +
    std::to_string(entry.baseId) + "," + std::to_string(entry.count) + "]";
  auto args = parser.parse(rawArgs).value();
  bool blockedByMpApi = false;

  if (auto wst = GetParent()) {
    const auto id = GetFormId();
    for (auto& listener : wst->listeners) {
      bool notBlocked = listener->OnMpApiEvent("onPutItem", args, id);
      blockedByMpApi = !notBlocked;
    }
  }

  return blockedByMpApi;
}

bool MpObjectReference::MpApiOnTakeItem(MpActor& source,
                                        const Inventory::Entry& entry)
{
  simdjson::dom::parser parser;
  std::string rawArgs = "[" + std::to_string(source.GetFormId()) + "," +
    std::to_string(entry.baseId) + "," + std::to_string(entry.count) + "]";
  auto args = parser.parse(rawArgs).value();
  bool blockedByMpApi = false;

  if (auto wst = GetParent()) {
    const auto id = GetFormId();
    for (auto& listener : wst->listeners) {
      bool notBlocked = listener->OnMpApiEvent("onTakeItem", args, id);
      blockedByMpApi = !notBlocked;
    }
  }

  return blockedByMpApi;
}

float MpObjectReference::GetTotalItemWeight() const
{
  const auto& entries = GetInventory().entries;
  const auto calculateWeight = [this](float sum,
                                      const Inventory::Entry& entry) {
    const auto& espm = GetParent()->GetEspm();
    const auto* record = espm.GetBrowser().LookupById(entry.baseId).rec;
    if (!record) {
      spdlog::warn(
        "MpObjectReference::GetTotalItemWeight of ({:x}): Record of form "
        "({}) is nullptr",
        GetFormId(), entry.baseId);
      return 0.f;
    }
    float weight = GetWeightFromRecord(record, GetParent()->GetEspmCache());
    if (!espm::utils::IsItem(record->GetType())) {
      spdlog::warn("Unsupported espm type {} has been detected, when "
                   "calculating overall weight.",
                   record->GetType().ToString());
    } else {
      spdlog::trace("Weight: {} for record of type {}", weight,
                    record->GetType().ToString());
    }
    return sum + entry.count * weight;
  };

  return std::accumulate(entries.begin(), entries.end(), 0.f, calculateWeight);
}
