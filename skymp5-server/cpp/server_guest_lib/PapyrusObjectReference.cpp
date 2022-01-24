#include "PapyrusObjectReference.h"

#include "EspmGameObject.h"
#include "FormCallbacks.h"
#include "MpActor.h"
#include "MpFormGameObject.h"
#include "MpObjectReference.h"
#include "SpSnippetFunctionGen.h"
#include "WorldState.h"
#include <cstring>

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

  if (auto formlist = espm::Convert<espm::FLST>(item.rec)) {
    formIds =
      espm::GetData<espm::FLST>(formlist->GetId(), selfRefr->GetParent())
        .formIds;
  } else {
    formIds.emplace_back(item.ToGlobalId(item.rec->GetId()));
    if (!silent && count > 0) {
      if (auto actor = dynamic_cast<MpActor*>(selfRefr)) {
        auto args = SpSnippetFunctionGen::SerializeArguments(arguments);
        (void)SpSnippet("SkympHacks", "AddItem", args.data()).Execute(actor);
      }
    }
  }

  for (auto itemId : formIds) {
    selfRefr->AddItem(itemId, count);
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

  if (!selfRefr || !item.rec || count <= 0)
    return VarValue::None();

  std::vector<uint32_t> formIds;

  if (auto formlist = espm::Convert<espm::FLST>(item.rec)) {
    formIds =
      espm::GetData<espm::FLST>(formlist->GetId(), selfRefr->GetParent())
        .formIds;
  } else {
    formIds.emplace_back(item.ToGlobalId(item.rec->GetId()));
    if (!silent && count > 0) {
      if (auto actor = dynamic_cast<MpActor*>(selfRefr)) {
        auto args = SpSnippetFunctionGen::SerializeArguments(arguments);
        (void)SpSnippet("SkympHacks", "RemoveItem", args.data()).Execute(actor);
      }
    }
  }

  for (auto itemId : formIds) {
    uint32_t maxCount = selfRefr->GetInventory().GetItemCount(itemId);
    uint32_t realCount = count > maxCount ? maxCount : count;
    selfRefr->RemoveItem(itemId, realCount, refrToAdd);
  }

  return VarValue::None();
}

VarValue PapyrusObjectReference::GetItemCount(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() >= 1) {
    auto selfRefr = GetFormPtr<MpObjectReference>(self);
    auto& form = GetRecordPtr(arguments[0]);

    const uint32_t formId = form.ToGlobalId(form.rec->GetId());

    if (selfRefr) {
      return VarValue(
        static_cast<int>(selfRefr->GetInventory().GetItemCount(formId)));
    }
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
      } else
        newRefr.reset(
          new MpObjectReference(locationalData, callbacks, baseId, type));

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
