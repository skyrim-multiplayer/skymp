#include "PapyrusObjectReference.h"

#include "FormCallbacks.h"
#include "LeveledListUtils.h"
#include "LocationalData.h"
#include "MpActor.h"
#include "MpObjectReference.h"
#include "SpSnippetFunctionGen.h"
#include "TimeUtils.h"
#include "WorldState.h"
#include "papyrus-vm/Structures.h"
#include "script_objects/EspmGameObject.h"
#include "script_objects/MpFormGameObject.h"
#include <algorithm>
#include <cstring>

namespace {
constexpr uint32_t kPlayerCharacterLevel = 1;
}

VarValue PapyrusObjectReference::IsHarvested(
  VarValue self, const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  return VarValue(selfRefr && selfRefr->IsHarvested());
}

VarValue PapyrusObjectReference::IsDisabled(
  VarValue self, const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  return VarValue(selfRefr && selfRefr->IsDisabled());
}

VarValue PapyrusObjectReference::IsDeleted(
  VarValue self, const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  return VarValue(selfRefr && selfRefr->IsDeleted());
}

VarValue PapyrusObjectReference::GetScale(
  VarValue self, const std::vector<VarValue>& arguments)
{
  return VarValue(1.f);
}

VarValue PapyrusObjectReference::SetScale(
  VarValue self, const std::vector<VarValue>& arguments)
{
  return VarValue::None();
}

VarValue PapyrusObjectReference::EnableNoWait(
  VarValue self, const std::vector<VarValue>& arguments)
{
  return VarValue::None();
}

VarValue PapyrusObjectReference::DisableNoWait(
  VarValue self, const std::vector<VarValue>& arguments)
{
  return VarValue::None();
}

namespace {
bool GetIsItemWithLightCarryableFlagChecked(
  WorldState* worldState, const espm::LookupResult& lookupRes)
{
  if (!lookupRes.rec) {
    return false;
  }

  if (!espm::utils::IsItem(lookupRes.rec->GetType())) {
    return false;
  }

  if (espm::utils::Is<espm::LIGH>(lookupRes.rec->GetType())) {
    auto res = espm::Convert<espm::LIGH>(lookupRes.rec)
                 ->GetData(worldState->GetEspmCache());
    bool isTorch = res.data.flags & espm::LIGH::Flags::CanBeCarried;
    if (!isTorch) {
      return false;
    }
  }

  return true;
}
}

VarValue PapyrusObjectReference::AddItem(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() < 3)
    return VarValue::None();
  const auto& item = GetRecordPtr(arguments[0]);
  auto count = static_cast<int>(arguments[1]);
  bool silent = static_cast<bool>(arguments[2].CastToBool());
  auto selfRefr = GetFormPtr<MpObjectReference>(self);

  auto worldState = selfRefr->GetParent();
  if (!worldState) {
    throw std::runtime_error("AddItem - no WorldState attached");
  }

  if (!selfRefr || !item.rec || count <= 0)
    return VarValue::None();

  if (!espm::utils::Is<espm::FLST>(item.rec->GetType()) &&
      !espm::utils::Is<espm::LVLI>(item.rec->GetType())) {
    if (!GetIsItemWithLightCarryableFlagChecked(worldState, item)) {
      throw std::runtime_error("AddItem - form is not an item");
    }
  }

  std::vector<uint32_t> formIds;
  bool runSkympHacks = false;

  if (auto formlist = espm::Convert<espm::FLST>(item.rec)) {
    auto data =
      espm::GetData<espm::FLST>(formlist->GetId(), selfRefr->GetParent());

    std::vector<uint32_t> tempFormIds = std::move(data.formIds);

    // Transform raw formIds into global formIds
    std::transform(
      tempFormIds.begin(), tempFormIds.end(), tempFormIds.begin(),
      [&](uint32_t rawFormId) { return item.ToGlobalId(rawFormId); });

    // Remove ids that belong to non-item records
    tempFormIds.erase(
      std::remove_if(tempFormIds.begin(), tempFormIds.end(),
                     [&](uint32_t formId) {
                       auto lookupRes =
                         worldState->GetEspm().GetBrowser().LookupById(formId);
                       return !GetIsItemWithLightCarryableFlagChecked(
                         worldState, lookupRes);
                     }),
      tempFormIds.end());

    formIds = std::move(tempFormIds);
  } else {
    formIds.emplace_back(item.ToGlobalId(item.rec->GetId()));
    runSkympHacks = true;
  }

  for (auto itemId : formIds) {
    auto resultItemLookupRes =
      worldState->GetEspm().GetBrowser().LookupById(itemId);
    auto leveledItem = espm::Convert<espm::LVLI>(resultItemLookupRes.rec);
    if (leveledItem) {
      const auto kCountMult = 1;
      auto map = LeveledListUtils::EvaluateListRecurse(
        worldState->GetEspm().GetBrowser(), resultItemLookupRes, kCountMult,
        kPlayerCharacterLevel);
      for (auto& p : map) {
        selfRefr->AddItem(p.first, p.second);
      }
    } else {
      selfRefr->AddItem(itemId, count);
    }
  }

  if (runSkympHacks) {
    if (!silent && count > 0) {
      if (auto actor = dynamic_cast<MpActor*>(selfRefr)) {
        auto args = SpSnippetFunctionGen::SerializeArguments(arguments);
        (void)SpSnippet("SkympHacks", "AddItem", args.data()).Execute(actor);
      }
    }
  }

  return VarValue::None();
}

VarValue PapyrusObjectReference::RemoveItem(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() < 4)
    return VarValue::None();

  const auto& item = GetRecordPtr(arguments[0]);
  auto count = static_cast<int>(arguments[1]);
  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  bool silent = static_cast<bool>(arguments[2].CastToBool());
  auto refrToAdd = GetFormPtr<MpObjectReference>(arguments[3]);

  auto worldState = selfRefr->GetParent();
  if (!worldState) {
    throw std::runtime_error("RemoveItem - no WorldState attached");
  }

  if (!selfRefr || !item.rec)
    return VarValue::None();

  if (!espm::utils::Is<espm::FLST>(item.rec->GetType())) {
    if (!espm::utils::IsItem(item.rec->GetType())) {
      throw std::runtime_error("RemoveItem - form is not an item");
    }
  }

  if (espm::utils::Is<espm::LIGH>(item.rec->GetType())) {
    auto res =
      espm::Convert<espm::LIGH>(item.rec)->GetData(worldState->GetEspmCache());
    bool isTorch = res.data.flags & espm::LIGH::Flags::CanBeCarried;
    if (!isTorch) {
      throw std::runtime_error(
        "RemoveItem - form is LIGH without CanBeCarried flag");
    }
  }

  std::vector<uint32_t> formIds;
  bool runSkympHacks = false;

  if (auto formlist = espm::Convert<espm::FLST>(item.rec)) {
    auto data =
      espm::GetData<espm::FLST>(formlist->GetId(), selfRefr->GetParent());

    std::vector<uint32_t> tempFormIds = data.formIds;

    // Transform raw formIds into global formIds
    std::transform(
      tempFormIds.begin(), tempFormIds.end(), tempFormIds.begin(),
      [&](uint32_t rawFormId) { return item.ToGlobalId(rawFormId); });

    // Remove ids that belong to non-item records
    tempFormIds.erase(
      std::remove_if(tempFormIds.begin(), tempFormIds.end(),
                     [&](uint32_t formId) {
                       auto lookupRes =
                         worldState->GetEspm().GetBrowser().LookupById(formId);
                       return !GetIsItemWithLightCarryableFlagChecked(
                         worldState, lookupRes);
                     }),
      tempFormIds.end());

    formIds = tempFormIds;
  } else {
    formIds.emplace_back(item.ToGlobalId(item.rec->GetId()));
    runSkympHacks = true;
  }

  for (auto itemId : formIds) {
    uint32_t maxCount = selfRefr->GetInventory().GetItemCount(itemId);
    uint32_t realCount = count > maxCount ? maxCount : count;
    selfRefr->RemoveItem(itemId, realCount, refrToAdd);
  }

  if (runSkympHacks) {
    if (!silent && count > 0) {
      if (auto actor = dynamic_cast<MpActor*>(selfRefr)) {
        auto args = SpSnippetFunctionGen::SerializeArguments(arguments);
        (void)SpSnippet("SkympHacks", "RemoveItem", args.data())
          .Execute(actor);
      }
    }
  }

  return VarValue::None();
}

VarValue PapyrusObjectReference::GetItemCount(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() >= 1) {
    auto selfRefr = GetFormPtr<MpObjectReference>(self);
    if (!selfRefr) {
      spdlog::warn("GetItemCount: self is not a reference");
      return VarValue(0);
    }
    auto& form = GetRecordPtr(arguments[0]);
    if (!form.rec) {
      spdlog::warn("GetItemCount: failed to extract form with GetRecordPtr");
      return VarValue(0);
    }
    std::vector<uint32_t> formIds;

    if (auto formlist = espm::Convert<espm::FLST>(form.rec)) {
      formIds =
        espm::GetData<espm::FLST>(formlist->GetId(), selfRefr->GetParent())
          .formIds;
    } else {
      formIds.emplace_back(form.ToGlobalId(form.rec->GetId()));
    }

    uint32_t count = 0;
    for (auto& formId : formIds) {
      count += selfRefr->GetInventory().GetItemCount(formId);
    }
    return VarValue(static_cast<int>(count));
  }
  return VarValue(0);
}

VarValue PapyrusObjectReference::GetAnimationVariableBool(
  VarValue self, const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);

  if (arguments.size() >= 1 &&
      arguments[0].GetType() == VarValue::kType_String) {
    auto animVarName = static_cast<const char*>(arguments[0]);
    return VarValue(selfRefr->GetAnimationVariableBool(animVarName));
  }
  return VarValue(false);
}

namespace {
void PlaceAtMeSpSnippet(MpObjectReference* self,
                        const std::vector<VarValue>& arguments)
{
  auto funcName = "PlaceAtMe";
  auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
  for (auto listener : self->GetListeners()) {
    auto targetRefr = dynamic_cast<MpActor*>(listener);
    if (targetRefr) {
      SpSnippet("ObjectReference", funcName, serializedArgs.data(),
                self->GetFormId())
        .Execute(targetRefr);
    }
  }
}
}

VarValue PapyrusObjectReference::PlaceAtMe(
  VarValue self, const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);

  if (!selfRefr || arguments.size() < 4) {
    return VarValue::None();
  }

  auto akFormToPlace = GetRecordPtr(arguments[0]);
  int aiCount = static_cast<int>(arguments[1].CastToInt());
  bool abForcePersist = static_cast<bool>(arguments[2].CastToBool());
  bool abInitiallyDisabled = static_cast<bool>(arguments[3].CastToBool());

  if (!akFormToPlace.rec) {
    return VarValue::None();
  }

  bool isExplosion = akFormToPlace.rec->GetType() == "EXPL";
  if (isExplosion) {
    // Well sp snippet fails ATM. and I don't want to overpollute clients and
    // network with those placeatme s for now

    // PlaceAtMeSpSnippet(selfRefr, arguments);

    // TODO: return pseudo-reference or even create real server-side form?
    return VarValue::None();
  }

  auto baseId = akFormToPlace.ToGlobalId(akFormToPlace.rec->GetId());

  float angleZDegrees = selfRefr->GetAngle().z;
  float angleZRadians = angleZDegrees; // TODO: fix Degrees/radians mismatch
  // TODO: support angleX, angleY
  LocationalData locationalData = { selfRefr->GetPos(),
                                    { 0, 0, angleZRadians },
                                    selfRefr->GetCellOrWorld() };
  FormCallbacks callbacks = selfRefr->GetCallbacks();
  std::string type = akFormToPlace.rec->GetType().ToString();

  std::unique_ptr<MpObjectReference> newRefr;

  if (akFormToPlace.rec->GetType() == "NPC_") {
    auto actor = new MpActor(locationalData, callbacks, baseId);
    newRefr.reset(actor);
  } else {
    newRefr.reset(
      new MpObjectReference(locationalData, callbacks, baseId, type));
  }

  auto worldState = selfRefr->GetParent();
  auto newRefrId = worldState->GenerateFormId();
  worldState->AddForm(std::move(newRefr), newRefrId);

  auto& refr = worldState->GetFormAt<MpObjectReference>(newRefrId);
  refr.ForceSubscriptionsUpdate();

  if (abInitiallyDisabled) {
    refr.Disable();
  }
  return VarValue(std::make_shared<MpFormGameObject>(&refr));
}

VarValue PapyrusObjectReference::SetAngle(
  VarValue self, const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  if (selfRefr && arguments.size() >= 3) {
    selfRefr->SetAngle(
      { static_cast<float>(static_cast<double>(arguments[0].CastToFloat())),
        static_cast<float>(static_cast<double>(arguments[1].CastToFloat())),
        static_cast<float>(static_cast<double>(arguments[2].CastToFloat())) });
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::Enable(VarValue self,
                                        const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  if (selfRefr) {
    selfRefr->Enable();
  }

  if (selfRefr->IsEspmForm() && !dynamic_cast<MpActor*>(selfRefr)) {
    auto funcName = "Enable";
    auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
    for (auto listener : selfRefr->GetListeners()) {
      auto targetRefr = dynamic_cast<MpActor*>(listener);
      if (targetRefr) {
        SpSnippet(GetName(), funcName, serializedArgs.data(),
                  selfRefr->GetFormId())
          .Execute(targetRefr);
      }
    }
  }

  return VarValue::None();
}

VarValue PapyrusObjectReference::Disable(
  VarValue self, const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  if (selfRefr) {
    selfRefr->Disable();
  }

  if (selfRefr->IsEspmForm() && !dynamic_cast<MpActor*>(selfRefr)) {
    auto funcName = "Disable";
    auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
    for (auto listener : selfRefr->GetListeners()) {
      auto targetRefr = dynamic_cast<MpActor*>(listener);
      if (targetRefr) {
        SpSnippet(GetName(), funcName, serializedArgs.data(),
                  selfRefr->GetFormId())
          .Execute(targetRefr);
      }
    }
  }

  return VarValue::None();
}

VarValue PapyrusObjectReference::Delete(VarValue self,
                                        const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  if (selfRefr) {
    selfRefr->Delete();
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::BlockActivation(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() < 1) {
    throw std::runtime_error("BlockActivation requires at least one argument");
  }
  bool block = static_cast<bool>(arguments[0].CastToBool());

  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  if (selfRefr) {
    selfRefr->SetActivationBlocked(block);
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::IsActivationBlocked(
  VarValue self, const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  return VarValue(selfRefr && selfRefr->IsActivationBlocked());
}

VarValue PapyrusObjectReference::Activate(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    if (arguments.size() < 2) {
      throw std::runtime_error("Activate requires at least two arguments");
    }
    auto akActivator = GetFormPtr<MpObjectReference>(arguments[0]);
    if (!akActivator) {
      spdlog::warn("Activate didn't recognize akActivator");
      // workaround for defaultPillarPuzzlelever script
      akActivator = selfRefr;
    }
    bool abDefaultProcessingOnly =
      static_cast<bool>(arguments[1].CastToBool());
    selfRefr->Activate(*akActivator, abDefaultProcessingOnly);
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::GetPositionX(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    return VarValue(selfRefr->GetPos().x);
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::GetPositionY(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    return VarValue(selfRefr->GetPos().y);
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::GetPositionZ(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    return VarValue(selfRefr->GetPos().z);
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::SetPosition(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    if (arguments.size() < 3) {
      throw std::runtime_error(
        "SetPosition requires at least three arguments");
    }
    // TODO: Add movement/position changing sync for ingame objects and remove
    // workaround below. Don't forget that SetPosition has fading effect. Also
    // note that this change is global. So players should get new position on
    // entering area or even outside of it
    selfRefr->SetPosAndAngleSilent(
      NiPoint3(
        static_cast<float>(static_cast<double>(arguments[0].CastToFloat())),
        static_cast<float>(static_cast<double>(arguments[1].CastToFloat())),
        static_cast<float>(static_cast<double>(arguments[2].CastToFloat()))),
      selfRefr->GetAngle());
    selfRefr->ForceSubscriptionsUpdate();
    auto funcName = "SetPosition";
    auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
    for (auto listener : selfRefr->GetListeners()) {
      auto targetRefr = dynamic_cast<MpActor*>(listener);
      if (targetRefr) {
        SpSnippet(GetName(), funcName, serializedArgs.data(),
                  selfRefr->GetFormId())
          .Execute(targetRefr);
      }
    }
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::GetBaseObject(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    auto baseId = selfRefr->GetBaseId();
    if (baseId) {
      if (auto worldState = selfRefr->GetParent()) {
        auto& espm = worldState->GetEspm();
        auto lookupRes = espm.GetBrowser().LookupById(baseId);
        if (lookupRes.rec) {
          return VarValue(std::make_shared<EspmGameObject>(lookupRes));
        }
      }
    }
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::PlayAnimation(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    if (arguments.size() < 1) {
      throw std::runtime_error("PlayAnimation requires at least 1 argument");
    }
    const char* animation = static_cast<const char*>(arguments[0]);
    selfRefr->SetLastAnimation(animation);

    auto funcName = "PlayAnimation";
    auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
    for (auto listener : selfRefr->GetListeners()) {
      auto targetRefr = dynamic_cast<MpActor*>(listener);
      if (targetRefr) {
        SpSnippet(GetName(), funcName, serializedArgs.data(),
                  selfRefr->GetFormId())
          .Execute(targetRefr);
      }
    }
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::PlayAnimationAndWait(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    if (arguments.size() < 1) {
      throw std::runtime_error("PlayAnimation requires at least 2 arguments");
    }

    const char* animation = static_cast<const char*>(arguments[0]);
    selfRefr->SetLastAnimation(animation);

    auto funcName = "PlayAnimationAndWait";
    auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);

    std::vector<Viet::Promise<VarValue>> promises;

    for (auto listener : selfRefr->GetListeners()) {
      auto targetRefr = dynamic_cast<MpActor*>(listener);
      if (targetRefr) {
        auto promise = SpSnippet(GetName(), funcName, serializedArgs.data(),
                                 selfRefr->GetFormId())
                         .Execute(targetRefr);
        promises.push_back(promise);
      }
    }

    if (promises.empty()) {
      // No listeners found
      return VarValue::None();
    }

    // Add 15 seconds timeout (protection against script freeze by user)
    // Code based on PapyrusUtility::Wait implementation
    auto time = Viet::TimeUtils::To<std::chrono::milliseconds>(15.f);
    auto timerPromise = selfRefr->GetParent()->SetTimer(time);
    auto resultPromise = Viet::Promise<VarValue>();
    timerPromise
      .Then([resultPromise](Viet::Void) {
        resultPromise.Resolve(VarValue::None());
      })
      .Catch([resultPromise](const char* e) { resultPromise.Reject(e); });
    promises.push_back(resultPromise);

    return VarValue(Viet::Promise<VarValue>::Any(promises));
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::PlayGamebryoAnimation(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    if (arguments.size() < 3) {
      throw std::runtime_error(
        "PlayGamebryoAnimation requires at least 3 arguments");
    }
    auto funcName = "PlayGamebryoAnimation";
    auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
    for (auto listener : selfRefr->GetListeners()) {
      auto targetRefr = dynamic_cast<MpActor*>(listener);
      if (targetRefr) {
        SpSnippet(GetName(), funcName, serializedArgs.data(),
                  selfRefr->GetFormId())
          .Execute(targetRefr);
      }
    }
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::MoveTo(VarValue self,
                                        const std::vector<VarValue>& arguments)
{
  if (arguments.size() < 1) {
    return VarValue::None();
  }
  auto* _thisObjectReference = GetFormPtr<MpObjectReference>(self);
  const auto* objectReference = GetFormPtr<MpObjectReference>(arguments[0]);
  auto* _thisActor = GetFormPtr<MpActor>(self);

  if (!_thisObjectReference) {
    return VarValue::None();
  }

  if (!objectReference) {
    spdlog::error("MoveTo - target object reference was nullptr");
    return VarValue::None();
  }

  const float xOffset = static_cast<float>(
                static_cast<double>(arguments[1].CastToFloat())),
              yOffset = static_cast<float>(
                static_cast<double>(arguments[2].CastToFloat())),
              zOffset = static_cast<float>(
                static_cast<double>(arguments[3].CastToFloat()));
  const bool matchRotation = static_cast<bool>(arguments[4].CastToBool());
  NiPoint3 dest = objectReference->GetPos();
  dest.x += xOffset;
  dest.y += yOffset;
  dest.z += zOffset;
  const NiPoint3 rotation = matchRotation ? objectReference->GetAngle()
                                          : _thisObjectReference->GetAngle();
  LocationalData data{ dest, rotation, objectReference->GetCellOrWorld() };
  if (_thisActor) {
    _thisActor->Teleport(data);
  } else {
    _thisObjectReference->SetCellOrWorld(objectReference->GetCellOrWorld());
    _thisObjectReference->SetAngle(rotation);
    _thisObjectReference->SetPos(dest);
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::SetOpen(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() < 1) {
    spdlog::error("SetOpen: not enough arguments");
    return VarValue::None();
  }
  auto* _thisObjectReference = GetFormPtr<MpObjectReference>(self);
  if (!_thisObjectReference) {
    spdlog::error("SetOpen: self is not a reference");
    return VarValue::None();
  }

  if (_thisObjectReference->GetBaseType() != "DOOR") {
    spdlog::error("SetOpen: self is not a door");
    return VarValue::None();
  }

  _thisObjectReference->SetOpen(static_cast<bool>(arguments[0].CastToBool()));
  return VarValue::None();
}

VarValue PapyrusObjectReference::Is3DLoaded(VarValue,
                                            const std::vector<VarValue>&)
{
  // stub
  return VarValue(true);
}

namespace LinkedRefUtils {
static VarValue GetLinkedRef(VarValue self)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    auto lookupRes = selfRefr->GetParent()->GetEspm().GetBrowser().LookupById(
      selfRefr->GetFormId());
    auto data =
      espm::GetData<espm::REFR>(selfRefr->GetFormId(), selfRefr->GetParent());
    if (data.linkedRefId) {
      auto& linkedRef = selfRefr->GetParent()->GetFormAt<MpObjectReference>(
        lookupRes.ToGlobalId(data.linkedRefId));
      return VarValue(std::make_shared<MpFormGameObject>(&linkedRef));
    }
  }

  return VarValue::None();
}
}

VarValue PapyrusObjectReference::GetLinkedRef(
  VarValue self, const std::vector<VarValue>& arguments)
{
  // TODO: implement keyword argument
  // https://ck.uesp.net/wiki/GetLinkedRef_-_ObjectReference
  if (arguments.size() > 0 && arguments[0] != VarValue::None()) {
    spdlog::warn(
      "GetLinkedRef doesn't support Keyword argument at this moment");
  }

  return LinkedRefUtils::GetLinkedRef(self);
}

VarValue PapyrusObjectReference::GetNthLinkedRef(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.empty()) {
    spdlog::error("GetNthLinkedRef - expected at least 1 argument");
    return VarValue::None();
  }
  int n = static_cast<int>(arguments[0]);
  if (n < 0) {
    return VarValue::None();
  }

  VarValue cursor = self;
  for (int i = 0; i < n; ++i) {
    cursor = LinkedRefUtils::GetLinkedRef(cursor);
  }
  return cursor;
}

VarValue PapyrusObjectReference::GetParentCell(VarValue self,
                                               const std::vector<VarValue>&)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    auto cellOrWorld =
      selfRefr->GetCellOrWorld().ToFormId(selfRefr->GetParent()->espmFiles);
    auto lookupRes =
      selfRefr->GetParent()->GetEspm().GetBrowser().LookupById(cellOrWorld);
    if (lookupRes.rec == nullptr) {
      spdlog::warn("GetParentCell - nullptr cell/world found");
    } else if (lookupRes.rec->GetType() == espm::WRLD::kType) {
      // TODO: support. at least you can use WRLD + x/y to find exterior cell
      spdlog::warn(
        "GetParentCell - exterior cells are not supported at this moment");
    } else if (lookupRes.rec->GetType() == espm::CELL::kType) {
      return VarValue(std::make_shared<EspmGameObject>(lookupRes));
    } else {
      spdlog::warn("GetParentCell - bad record type {}",
                   lookupRes.rec->GetType().ToString());
    }
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::GetOpenState(VarValue self,
                                              const std::vector<VarValue>&)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    if (selfRefr->GetBaseType() == "DOOR") {
      return selfRefr->IsOpen() ? VarValue(1) : VarValue(3);
    }
  }
  return VarValue(0);
}

VarValue PapyrusObjectReference::GetAllItemsCount(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    int total = static_cast<int>(selfRefr->GetInventory().GetTotalItemCount());
    return VarValue(total);
  }
  return VarValue(0);
}

VarValue PapyrusObjectReference::IsContainerEmpty(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    return VarValue(selfRefr->GetInventory().IsEmpty());
  }
  return VarValue(0);
}

VarValue PapyrusObjectReference::SetDisplayName(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {
    if (arguments.size() < 2) {
      throw std::runtime_error("SetDisplayName requires at least 2 arguments");
    }
    const char* displayName = static_cast<const char*>(arguments[0]);
    selfRefr->SetDisplayName(displayName);

    bool force = static_cast<bool>(arguments[1]);
    std::ignore = force;

    auto funcName = "SetDisplayName";
    auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
    for (auto listener : selfRefr->GetListeners()) {
      auto targetRefr = dynamic_cast<MpActor*>(listener);
      if (targetRefr) {
        SpSnippet(GetName(), funcName, serializedArgs.data(),
                  selfRefr->GetFormId())
          .Execute(targetRefr);
      }
    }
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::GetDistance(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (auto selfRefr = GetFormPtr<MpObjectReference>(self)) {

    if (arguments.size() < 1) {
      spdlog::error("GetDistance requires at least 1 argument");
      return VarValue(0.f);
    }

    if (auto other = GetFormPtr<MpObjectReference>(arguments[0])) {
      if (selfRefr->GetCellOrWorld() == other->GetCellOrWorld()) {
        return VarValue((other->GetPos() - selfRefr->GetPos()).Length());
      } else {
        // There must be "very large number" according to wiki
        return VarValue(1'000'000'000.f);
      }
    }
  }

  return VarValue(0.f);
}

void PapyrusObjectReference::Register(
  VirtualMachine& vm, std::shared_ptr<IPapyrusCompatibilityPolicy> policy)
{
  AddMethod(vm, "isHarvested", &PapyrusObjectReference::IsHarvested);
  AddMethod(vm, "isDisabled", &PapyrusObjectReference::IsDisabled);
  AddMethod(vm, "isDeleted", &PapyrusObjectReference::IsDeleted);
  AddMethod(vm, "getScale", &PapyrusObjectReference::GetScale);
  AddMethod(vm, "setScale", &PapyrusObjectReference::SetScale);
  AddMethod(vm, "enableNoWait", &PapyrusObjectReference::EnableNoWait);
  AddMethod(vm, "disableNoWait", &PapyrusObjectReference::DisableNoWait);
  AddMethod(vm, "addItem", &PapyrusObjectReference::AddItem);
  AddMethod(vm, "removeItem", &PapyrusObjectReference::RemoveItem);
  AddMethod(vm, "getItemCount", &PapyrusObjectReference::GetItemCount);
  AddMethod(vm, "getAnimationVariableBool",
            &PapyrusObjectReference::GetAnimationVariableBool);
  AddMethod(vm, "placeAtMe", &PapyrusObjectReference::PlaceAtMe);
  AddMethod(vm, "setAngle", &PapyrusObjectReference::SetAngle);
  AddMethod(vm, "enable", &PapyrusObjectReference::Enable);
  AddMethod(vm, "disable", &PapyrusObjectReference::Disable);
  AddMethod(vm, "delete", &PapyrusObjectReference::Delete);
  AddMethod(vm, "blockActivation", &PapyrusObjectReference::BlockActivation);
  AddMethod(vm, "isActivationBlocked",
            &PapyrusObjectReference::IsActivationBlocked);
  AddMethod(vm, "activate", &PapyrusObjectReference::Activate);
  AddMethod(vm, "getPositionX", &PapyrusObjectReference::GetPositionX);
  AddMethod(vm, "getPositionY", &PapyrusObjectReference::GetPositionY);
  AddMethod(vm, "getPositionZ", &PapyrusObjectReference::GetPositionZ);
  AddMethod(vm, "setPosition", &PapyrusObjectReference::SetPosition);
  AddMethod(vm, "getBaseObject", &PapyrusObjectReference::GetBaseObject);
  AddMethod(vm, "playAnimation", &PapyrusObjectReference::PlayAnimation);
  AddMethod(vm, "playAnimationAndWait",
            &PapyrusObjectReference::PlayAnimationAndWait);
  AddMethod(vm, "playGamebryoAnimation",
            &PapyrusObjectReference::PlayGamebryoAnimation);
  AddMethod(vm, "moveTo", &PapyrusObjectReference::MoveTo);
  AddMethod(vm, "setOpen", &PapyrusObjectReference::SetOpen);
  AddMethod(vm, "is3DLoaded", &PapyrusObjectReference::Is3DLoaded);
  AddMethod(vm, "getLinkedRef", &PapyrusObjectReference::GetLinkedRef);
  AddMethod(vm, "getNthLinkedRef", &PapyrusObjectReference::GetNthLinkedRef);
  AddMethod(vm, "getParentCell", &PapyrusObjectReference::GetParentCell);
  AddMethod(vm, "getOpenState", &PapyrusObjectReference::GetOpenState);
  AddMethod(vm, "getAllItemsCount", &PapyrusObjectReference::GetAllItemsCount);
  AddMethod(vm, "isContainerEmpty", &PapyrusObjectReference::IsContainerEmpty);
  AddMethod(vm, "setDisplayName", &PapyrusObjectReference::SetDisplayName);
  AddMethod(vm, "getDistance", &PapyrusObjectReference::GetDistance);
}
