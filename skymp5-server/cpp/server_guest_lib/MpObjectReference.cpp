#include "MpObjectReference.h"
#include "ChangeFormGuard.h"
#include "EspmGameObject.h"
#include "FormCallbacks.h"
#include "LeveledListUtils.h"
#include "MpActor.h"
#include "MpChangeForms.h"
#include "MsgType.h"
#include "PapyrusGame.h"
#include "PapyrusObjectReference.h"
#include "Primitive.h"
#include "ScopedTask.h"
#include "ScriptStorage.h"
#include "ScriptVariablesHolder.h"
#include "WorldState.h"
#include "libespm/GroupUtils.h"
#include "papyrus-vm/Reader.h"
#include "papyrus-vm/VirtualMachine.h"
#include <map>
#include <optional>

constexpr uint32_t kPlayerCharacterLevel = 1;

std::string MpObjectReference::CreatePropertyMessage(
  MpObjectReference* self, const char* name, const nlohmann::json& value)
{
  std::string str;
  str += Networking::MinPacketId;
  str += PreparePropertyMessage(self, name, value).dump();
  return str;
}

nlohmann::json MpObjectReference::PreparePropertyMessage(
  MpObjectReference* self, const char* name, const nlohmann::json& value)
{
  std::string baseRecordType;

  auto& loader = self->GetParent()->GetEspm();
  auto base = loader.GetBrowser().LookupById(GetBaseId());
  if (base.rec) {
    baseRecordType = base.rec->GetType().ToString();
  }

  auto object = nlohmann::json{ { "idx", self->GetIdx() },
                                { "t", MsgType::UpdateProperty },
                                { "propName", name },
                                { "refrId", self->GetFormId() },
                                { "data", value } };

  // See 'perf: improve game framerate #1186'
  // Client needs to know if it is DOOR or not
  if (baseRecordType == "DOOR") {
    object["baseRecordType"] = baseRecordType;
  }

  return object;
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

std::chrono::system_clock::duration MpObjectReference::GetRelootTime() const
{
  if (relootTimeOverride)
    return *relootTimeOverride;

  if (auto time = GetParent()->GetRelootTime(baseType))
    return *time;

  if (!strcmp(baseType.data(), "FLOR") || !strcmp(baseType.data(), "TREE")) {
    return std::chrono::hours(1);
  } else if (!strcmp(baseType.data(), "DOOR")) {
    return std::chrono::seconds(3);
  } else if (espm::IsItem(baseType.data())) {
    return std::chrono::hours(1);
  } else if (!strcmp(baseType.data(), "CONT")) {
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
  if (IsHarvested())
    visitor("isHarvested", "true");
  if (IsOpen())
    visitor("isOpen", "true");
  if (mode == VisitPropertiesMode::All && !GetInventory().IsEmpty()) {
    auto inventoryDump = GetInventory().ToJson().dump();
    visitor("inventory", inventoryDump.data());
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
  if (auto worldState = activationSource.GetParent(); worldState->HasEspm()) {
    CheckInteractionAbility(activationSource);
  }

  bool activationBlockedByMpApi = MpApiOnActivate(activationSource);

  if (!activationBlockedByMpApi &&
      (!activationBlocked || defaultProcessingOnly))
    ProcessActivate(activationSource);

  if (!defaultProcessingOnly) {
    auto arg = activationSource.ToVarValue();
    SendPapyrusEvent("OnActivate", &arg, 1);
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
          wst->SetTimer(0).Then([wst, id, inside, me, myId, this](Viet::Void) {
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
  RemoveItems({ e }, &ac);

  if (GetInventory().IsEmpty()) {
    RequestReloot();
  }
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

void MpObjectReference::Disable()
{
  if (ChangeForm().isDisabled)
    return;

  EditChangeForm(
    [&](MpChangeFormREFR& changeForm) { changeForm.isDisabled = true; });
  RemoveFromGrid();
}

void MpObjectReference::Enable()
{
  if (!ChangeForm().isDisabled)
    return;

  EditChangeForm(
    [&](MpChangeFormREFR& changeForm) { changeForm.isDisabled = false; });
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
    // to self-subscribe twice! We have already been subscribed to self in the
    // last line of code
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
    if (listenerAsActor)
      this->SendPropertyTo(newHosterId != 0 &&
                               newHosterId != listener->GetFormId()
                             ? hostedMsg
                             : notHostedMsg,
                           *listenerAsActor);
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

  auto baseItem = VarValue(static_cast<int32_t>(baseId));
  auto itemCount = VarValue(static_cast<int32_t>(count));
  auto itemReference = VarValue((IGameObject*)nullptr);
  auto sourceContainer = VarValue((IGameObject*)nullptr);
  VarValue args[4] = { baseItem, itemCount, itemReference, sourceContainer };
  SendPapyrusEvent("OnItemAdded", args, 4);
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

  for (const auto& entri : entries) {
    auto baseItem = VarValue(static_cast<int32_t>(entri.baseId));
    auto itemCount = VarValue(static_cast<int32_t>(entri.count));
    auto itemReference = VarValue((IGameObject*)nullptr);
    auto sourceContainer = VarValue((IGameObject*)nullptr);
    VarValue args[4] = { baseItem, itemCount, itemReference, sourceContainer };
    SendPapyrusEvent("OnItemAdded", args, 4);
  }
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
  if (profileId < 0)
    throw std::runtime_error("Invalid profileId passed to RegisterProfileId");

  if (ChangeForm().profileId >= 0)
    throw std::runtime_error("Already has a valid profileId");

  EditChangeForm(
    [&](MpChangeFormREFR& changeForm) { changeForm.profileId = profileId; });
  GetParent()->actorIdByProfileId[profileId].insert(GetFormId());
}

void MpObjectReference::Subscribe(MpObjectReference* emitter,
                                  MpObjectReference* listener)
{
  const bool emitterIsActor = !!dynamic_cast<MpActor*>(emitter);
  const bool listenerIsActor = !!dynamic_cast<MpActor*>(listener);
  if (!emitterIsActor && !listenerIsActor)
    return;

  if (!emitter->pImpl->onInitEventSent &&
      listener->GetChangeForm().profileId != -1) {
    emitter->pImpl->onInitEventSent = true;
    emitter->SendPapyrusEvent("OnInit");
  }

  const bool hasPrimitive = emitter->HasPrimitive();

  emitter->InitListenersAndEmitters();
  listener->InitListenersAndEmitters();
  emitter->listeners->insert(listener);
  listener->emitters->insert(emitter);
  if (!hasPrimitive)
    emitter->callbacks->subscribe(emitter, listener);

  if (hasPrimitive) {
    if (!listener->emittersWithPrimitives)
      listener->emittersWithPrimitives.reset(new std::map<uint32_t, bool>);
    listener->emittersWithPrimitives->insert({ emitter->GetFormId(), false });
  }
}

void MpObjectReference::Unsubscribe(MpObjectReference* emitter,
                                    MpObjectReference* listener)
{
  bool bothNonActors =
    !dynamic_cast<MpActor*>(emitter) && !dynamic_cast<MpActor*>(listener);
  if (bothNonActors)
    return;

  const bool hasPrimitive = emitter->HasPrimitive();

  if (!hasPrimitive)
    emitter->callbacks->unsubscribe(emitter, listener);
  emitter->listeners->erase(listener);
  listener->emitters->erase(emitter);

  if (listener->emittersWithPrimitives && hasPrimitive) {
    listener->emittersWithPrimitives->erase(emitter->GetFormId());
  }
}

const std::set<MpObjectReference*>& MpObjectReference::GetListeners() const
{
  static const std::set<MpObjectReference*> g_emptyListeners;
  return listeners ? *listeners : g_emptyListeners;
}

const std::set<MpObjectReference*>& MpObjectReference::GetEmitters() const
{
  static const std::set<MpObjectReference*> g_emptyEmitters;
  return emitters ? *emitters : g_emptyEmitters;
}

void MpObjectReference::RequestReloot(
  std::optional<std::chrono::system_clock::duration> time)
{
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
  if (ChangeForm().nextRelootDatetime)
    res.reset(new std::chrono::time_point<std::chrono::system_clock>(
      std::chrono::system_clock::from_time_t(
        ChangeForm().nextRelootDatetime)));
  return res;
}

MpChangeForm MpObjectReference::GetChangeForm() const
{
  MpChangeForm res;
  static_cast<MpChangeFormREFR&>(res) = ChangeForm();

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
    std::stringstream ss;
    ss << "Anomally, baseId should never change (";
    ss << std::hex << currentBaseId << " => " << newBaseId << ")";
    throw std::runtime_error(ss.str());
  }

  if (ChangeForm().formDesc != changeForm.formDesc) {
    throw std::runtime_error("Expected formDesc to be " +
                             ChangeForm().formDesc.ToString() +
                             ", but found " + changeForm.formDesc.ToString());
  }

  // Perform all required grid operations
  changeForm.isDisabled ? Disable() : Enable();
  SetCellOrWorldObsolete(changeForm.worldOrCellDesc);
  SetPos(changeForm.position);

  if (changeForm.profileId >= 0)
    RegisterProfileId(changeForm.profileId);

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

  // It crashed during sparsepp hashmap indexing.
  // Not sure why. And not sure why this code actually been here.
  // It seems that MoveOnGrid will be caled later.
  /*if (!IsDisabled()) {
    auto& gridInfo = GetParent()->grids[ChangeForm().worldOrCell];
    MoveOnGrid(*gridInfo.grid);
  }*/

  // We should queue created form for saving as soon as it is initialized
  const auto mode = (!hasChangeForm && formId >= 0xff000000)
    ? Mode::RequestSave
    : Mode::NoRequestSave;

  EditChangeForm(
    [&](MpChangeFormREFR& changeForm) {
      changeForm.formDesc =
        FormDesc::FromFormId(formId, GetParent()->espmFiles);
    },
    mode);
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

  if (t == espm::TREE::kType || t == espm::FLOR::kType || espm::IsItem(t)) {
    if (!IsHarvested()) {
      auto mapping = loader.GetBrowser().GetCombMapping(base.fileIdx);
      uint32_t resultItem = 0;
      if (t == espm::TREE::kType) {
        espm::FLOR::Data data;
        data =
          espm::Convert<espm::TREE>(base.rec)->GetData(compressedFieldsCache);
        resultItem = espm::GetMappedId(data.resultItem, *mapping);
      } else if (t == espm::FLOR::kType) {
        espm::FLOR::Data data;
        data =
          espm::Convert<espm::FLOR>(base.rec)->GetData(compressedFieldsCache);
        resultItem = espm::GetMappedId(data.resultItem, *mapping);
      } else {
        resultItem = espm::GetMappedId(base.rec->GetId(), *mapping);
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
        uint32_t count =
          refrRecord ? refrRecord->GetData(compressedFieldsCache).count : 1;
        activationSource.AddItem(resultItem, count ? count : 1);
      }
      SetHarvested(true);
      RequestReloot();
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

      static const auto g_pi = std::acos(-1.f);
      const NiPoint3 rot = { teleport->rotRadians[0] / g_pi * 180,
                             teleport->rotRadians[1] / g_pi * 180,
                             teleport->rotRadians[2] / g_pi * 180 };

      std::string msg;
      msg += Networking::MinPacketId;
      msg += nlohmann::json{
        { "pos", { teleport->pos[0], teleport->pos[1], teleport->pos[2] } },
        { "rot", { rot[0], rot[1], rot[2] } },
        { "worldOrCell", teleportWorldOrCell },
        { "type", "teleport" }
      }.dump();
      if (actorActivator)
        actorActivator->SendToUser(msg.data(), msg.size(), true);

      activationSource.SetCellOrWorldObsolete(
        FormDesc::FromFormId(teleportWorldOrCell, worldState->espmFiles));
      activationSource.SetPos(
        { teleport->pos[0], teleport->pos[1], teleport->pos[2] });
      activationSource.SetAngle(rot);

    } else {
      SetOpen(!IsOpen());
    }
  } else if (t == espm::CONT::kType && actorActivator) {
    EnsureBaseContainerAdded(loader);
    if (!this->occupant) {
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

void MpObjectReference::RemoveFromGrid()
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
  if (!baseId || !GetParent()->espm)
    return;

  auto scriptStorage = GetParent()->GetScriptStorage();
  if (!scriptStorage)
    return;

  auto& compressedFieldsCache = GetParent()->GetEspmCache();

  std::vector<std::string> scriptNames;

  auto& br = GetParent()->espm->GetBrowser();
  auto base = br.LookupById(baseId);
  auto refr = br.LookupById(GetFormId());
  for (auto record : { base.rec, refr.rec }) {
    if (!record)
      continue;
    espm::ScriptData scriptData;
    record->GetScriptData(&scriptData, compressedFieldsCache);

    auto& scriptsInStorage =
      GetParent()->GetScriptStorage()->ListScripts(false);
    for (auto& script : scriptData.scripts) {
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

  if (!scriptNames.empty()) {
    pImpl->scriptState.reset(new ScriptState);

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
  }
}

void MpObjectReference::SendInventoryUpdate()
{
  auto actor = dynamic_cast<MpActor*>(this);
  if (actor) {
    std::string msg;
    msg += Networking::MinPacketId;
    msg += nlohmann::json{
      { "inventory", actor->GetInventory().ToJson() },
      { "type", "setInventory" }
    }.dump();
    actor->SendToUser(msg.data(), msg.size(), true);
  }
}

void MpObjectReference::SendOpenContainer(uint32_t targetId)
{
  auto actor = dynamic_cast<MpActor*>(this);
  if (actor) {
    std::string msg;
    msg += Networking::MinPacketId;
    msg += nlohmann::json{
      { "target", targetId }, { "type", "openContainer" }
    }.dump();
    actor->SendToUser(msg.data(), msg.size(), true);
  }
}

std::vector<espm::CONT::ContainerObject> GetOutfitObjects(
  const espm::CombineBrowser& br, const espm::LookupResult& lookupRes,
  espm::CompressedFieldsCache& compressedFieldsCache)
{
  std::vector<espm::CONT::ContainerObject> res;

  if (auto baseNpc = espm::Convert<espm::NPC_>(lookupRes.rec)) {
    auto data = baseNpc->GetData(compressedFieldsCache);

    auto outfitId = lookupRes.ToGlobalId(data.defaultOutfitId);
    auto outfit = espm::Convert<espm::OTFT>(br.LookupById(outfitId).rec);
    auto outfitData =
      outfit ? outfit->GetData(compressedFieldsCache) : espm::OTFT::Data();

    for (uint32_t i = 0; i != outfitData.count; ++i) {
      auto outfitElementId = lookupRes.ToGlobalId(outfitData.formIds[i]);
      res.push_back({ outfitElementId, 1 });
    }
  }
  return res;
}

std::vector<espm::CONT::ContainerObject> GetInventoryObjects(
  const espm::CombineBrowser& br, const espm::LookupResult& lookupRes,
  espm::CompressedFieldsCache& compressedFieldsCache)
{
  auto baseContainer = espm::Convert<espm::CONT>(lookupRes.rec);
  if (baseContainer)
    return baseContainer->GetData(compressedFieldsCache).objects;

  auto baseNpc = espm::Convert<espm::NPC_>(lookupRes.rec);
  if (baseNpc) {
    return baseNpc->GetData(compressedFieldsCache).objects;
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
    for (auto& p : map)
      (*itemsToAdd)[p.first] += p.second;
  } else {
    (*itemsToAdd)[entry.formId] += entry.count;
  }
}

void MpObjectReference::EnsureBaseContainerAdded(espm::Loader& espm)
{
  if (ChangeForm().baseContainerAdded)
    return;

  auto worldState = GetParent();
  if (!worldState)
    return;

  auto lookupRes = espm.GetBrowser().LookupById(GetBaseId());

  std::map<uint32_t, uint32_t> itemsToAdd, itemsToEquip;

  auto inventoryObjects = GetInventoryObjects(espm.GetBrowser(), lookupRes,
                                              worldState->GetEspmCache());
  for (auto& entry : inventoryObjects) {
    AddContainerObject(entry, &itemsToAdd);
  }

  auto outfitObjects =
    GetOutfitObjects(espm.GetBrowser(), lookupRes, worldState->GetEspmCache());
  for (auto& entry : outfitObjects) {
    AddContainerObject(entry, &itemsToAdd);
    AddContainerObject(entry, &itemsToEquip);
  }
  if (auto actor = dynamic_cast<MpActor*>(this)) {
    Equipment eq;
    for (auto p : itemsToEquip) {
      Inventory::Entry e;
      e.baseId = p.first;
      e.count = p.second;
      e.extra.worn = Inventory::Worn::Right;
      eq.inv.AddItems({ e });
    }
    actor->SetEquipment(eq.ToJson().dump());
  }

  std::vector<Inventory::Entry> entries;
  for (auto& p : itemsToAdd)
    entries.push_back({ p.first, p.second });
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
  auto str = CreatePropertyMessage(this, name, value);
  for (auto listener : GetListeners()) {
    auto listenerAsActor = dynamic_cast<MpActor*>(listener);
    if (listenerAsActor)
      listenerAsActor->SendToUser(str.data(), str.size(), true);
  }
}

void MpObjectReference::SendPropertyTo(const char* name,
                                       const nlohmann::json& value,
                                       MpActor& target)
{
  auto str = CreatePropertyMessage(this, name, value);
  SendPropertyTo(str, target);
}

void MpObjectReference::SendPropertyTo(const std::string& preparedPropMsg,
                                       MpActor& target)
{
  target.SendToUser(preparedPropMsg.data(), preparedPropMsg.size(), true);
}

void MpObjectReference::BeforeDestroy()
{
  if (this->occupant && this->occupantDestroySink)
    this->occupant->RemoveEventSink(this->occupantDestroySink);

  // Move far far away calling OnTriggerExit, unsubscribing, etc
  SetPos({ -1'000'000'000, 0, 0 });

  MpForm::BeforeDestroy();

  RemoveFromGrid();
}
