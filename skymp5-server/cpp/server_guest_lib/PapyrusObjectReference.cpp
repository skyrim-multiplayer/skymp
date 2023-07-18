#include "PapyrusObjectReference.h"

#include "EspmGameObject.h"
#include "FormCallbacks.h"
#include "MpActor.h"
#include "MpFormGameObject.h"
#include "MpObjectReference.h"
#include "SpSnippetFunctionGen.h"
#include "WorldState.h"
#include <cstring>

VarValue PapyrusObjectReference::IsHarvested(
  VarValue self, const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  return VarValue(selfRefr && selfRefr->IsHarvested());
}

VarValue PapyrusObjectReference::IsDisabled(
  VarValue self, const std::vector<VarValue>& arguments)
{
  return VarValue(false);
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

VarValue PapyrusObjectReference::Delete(VarValue self,
                                        const std::vector<VarValue>& arguments)
{
  return VarValue::None();
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

  if (!selfRefr || !item.rec || count <= 0)
    return VarValue::None();

  std::vector<uint32_t> formIds;
  bool runSkympHacks = false;

  if (auto formlist = espm::Convert<espm::FLST>(item.rec)) {
    formIds =
      espm::GetData<espm::FLST>(formlist->GetId(), selfRefr->GetParent())
        .formIds;
  } else {
    formIds.emplace_back(item.ToGlobalId(item.rec->GetId()));
    runSkympHacks = true;
  }

  for (auto itemId : formIds) {
    selfRefr->AddItem(itemId, count);
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

  if (!selfRefr || !item.rec)
    return VarValue::None();

  std::vector<uint32_t> formIds;
  bool runSkympHacks = false;

  if (auto formlist = espm::Convert<espm::FLST>(item.rec)) {
    formIds =
      espm::GetData<espm::FLST>(formlist->GetId(), selfRefr->GetParent())
        .formIds;
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

VarValue PapyrusObjectReference::PlaceAtMe(
  VarValue self, const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);

  if (selfRefr && arguments.size() >= 4) {
    auto akFormToPlace = GetRecordPtr(arguments[0]);
    int aiCount = static_cast<int>(arguments[1].CastToInt());
    bool abForcePersist = static_cast<bool>(arguments[2].CastToBool());
    bool abInitiallyDisabled = static_cast<bool>(arguments[3].CastToBool());

    if (akFormToPlace.rec) {
      auto baseId = akFormToPlace.ToGlobalId(akFormToPlace.rec->GetId());

      LocationalData locationalData = {
        selfRefr->GetPos(),
        { 0, 0, selfRefr->GetAngle().z }, // TODO: fix Degrees/radians mismatch
        selfRefr->GetCellOrWorld()
      };
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
      return VarValue(std::make_shared<MpFormGameObject>(&refr));
    }
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::SetAngle(
  VarValue self, const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  if (selfRefr && arguments.size() >= 3) {
    selfRefr->SetAngle(
      { (float)static_cast<double>(arguments[0].CastToFloat()),
        (float)static_cast<double>(arguments[1].CastToFloat()),
        (float)static_cast<double>(arguments[2].CastToFloat()) });
  }
  return VarValue::None();
}

VarValue PapyrusObjectReference::Enable(VarValue self,
                                        const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  if (selfRefr)
    selfRefr->Enable();
  return VarValue::None();
}

VarValue PapyrusObjectReference::Disable(
  VarValue self, const std::vector<VarValue>& arguments)
{
  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  if (selfRefr)
    selfRefr->Disable();
  return VarValue::None();
}

VarValue PapyrusObjectReference::BlockActivation(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() < 1)
    throw std::runtime_error("BlockActivation requires at least one argument");
  bool block = static_cast<bool>(arguments[0].CastToBool());

  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  if (selfRefr)
    selfRefr->SetActivationBlocked(block);
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
    if (arguments.size() < 2)
      throw std::runtime_error("Activate requires at least two arguments");
    auto akActivator = GetFormPtr<MpObjectReference>(arguments[0]);
    if (!akActivator)
      throw std::runtime_error("Activate didn't recognize akActivator");
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
