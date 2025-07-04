#include "PapyrusGame.h"
#include "PapyrusFormList.h"

#include "SpSnippetFunctionGen.h"
#include "WorldState.h"
#include "libespm/Combiner.h"
#include "script_objects/EspmGameObject.h"
#include "script_objects/MpFormGameObject.h"

VarValue PapyrusGame::ForceThirdPerson(VarValue self,
                                       const std::vector<VarValue>& arguments)
{
  return ExecuteSpSnippetAndGetPromise(GetName(), "ForceThirdPerson",
                                       compatibilityPolicy, self, arguments);
}

VarValue PapyrusGame::DisablePlayerControls(
  VarValue self, const std::vector<VarValue>& arguments)
{
  return ExecuteSpSnippetAndGetPromise(GetName(), "DisablePlayerControls",
                                       compatibilityPolicy, self, arguments);
}

VarValue PapyrusGame::EnablePlayerControls(
  VarValue self, const std::vector<VarValue>& arguments)
{
  return ExecuteSpSnippetAndGetPromise(GetName(), "EnablePlayerControls",
                                       compatibilityPolicy, self, arguments);
}

VarValue PapyrusGame::IncrementStat(VarValue self,
                                    const std::vector<VarValue>& arguments)
{
  return VarValue::None();
}

namespace {
bool ExistsInFormList(const VarValue& formList, uint32_t baseId)
{
  int n = static_cast<int>(PapyrusFormList().GetSize(formList, {}));
  for (int i = 0; i < n; ++i) {
    VarValue element = PapyrusFormList().GetAt(formList, { VarValue(i) });
    auto elementRec = GetRecordPtr(element);
    if (elementRec.rec &&
        baseId == elementRec.ToGlobalId(elementRec.rec->GetId())) {
      return true;
    }
  }
  return false;
}
}

namespace {
VarValue FindClosestReferenceHelper(
  MpObjectReference* arCenter, double afRadius,
  std::function<bool(MpObjectReference*)> criteria)
{
  if (!arCenter) {
    spdlog::warn("FindClosestReferenceHelper - arCenter is nullptr");
    return VarValue::None();
  }

  // why not < 0? because of NaN
  if (!(afRadius >= 0)) {
    spdlog::warn("FindClosestReferenceHelper - expected afRadius to be >= 0");
    return VarValue::None();
  }

  float bestDistance = std::numeric_limits<float>::infinity();
  MpObjectReference* bestNeighbour = nullptr;

  arCenter->VisitNeighbours([&](MpObjectReference* neighbour) {
    if (!criteria(neighbour)) {
      return;
    }

    float distance = (arCenter->GetPos() - neighbour->GetPos()).SqrLength();
    if (distance > afRadius * afRadius) {
      return;
    }

    if (bestDistance > distance) {
      bestDistance = distance;
      bestNeighbour = neighbour;
    }
  });

  if (bestNeighbour) {
    return VarValue(std::make_shared<MpFormGameObject>(bestNeighbour));
  }

  return VarValue::None();
}
}

VarValue PapyrusGame::FindClosestReferenceOfAnyTypeInListFromRef(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() >= 3) {
    auto arBaseObjects = arguments[0];
    auto arCenter = GetFormPtr<MpObjectReference>(arguments[1]);
    double afRadius = static_cast<double>(arguments[2].CastToFloat());

    return FindClosestReferenceHelper(
      arCenter, afRadius, [&](MpObjectReference* neighbour) {
        auto baseId = neighbour->GetBaseId();
        return ExistsInFormList(arBaseObjects, baseId);
      });
  }
  return VarValue::None();
}

VarValue PapyrusGame::FindClosestReferenceOfTypeFromRef(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() >= 3) {
    auto arBaseObject = GetRecordPtr(arguments[0]);
    auto arCenter = GetFormPtr<MpObjectReference>(arguments[1]);
    double afRadius = static_cast<double>(arguments[2].CastToFloat());

    return FindClosestReferenceHelper(
      arCenter, afRadius, [&](MpObjectReference* neighbour) {
        auto baseId = neighbour->GetBaseId();
        return arBaseObject.rec &&
          baseId == arBaseObject.ToGlobalId(arBaseObject.rec->GetId());
      });
  }
  return VarValue::None();
}

VarValue PapyrusGame::GetPlayer(VarValue self,
                                const std::vector<VarValue>& arguments)
{
  auto actor = compatibilityPolicy->GetDefaultActor("Game", "GetPlayer",
                                                    self.GetMetaStackId());

  if (!actor) {
    return VarValue::None();
  }

  return VarValue(std::make_shared<MpFormGameObject>(actor));
}

VarValue PapyrusGame::ShowRaceMenu(VarValue self,
                                   const std::vector<VarValue>& arguments)
{
  auto funcName = "ShowRaceMenu";
  RaceMenuHelper(self, funcName, arguments);
  return VarValue::None();
}

VarValue PapyrusGame::ShowLimitedRaceMenu(
  VarValue self, const std::vector<VarValue>& arguments)
{
  auto funcName = "ShowLimitedRaceMenu";
  RaceMenuHelper(self, funcName, arguments);
  return VarValue::None();
}

VarValue PapyrusGame::GetCameraState(VarValue self,
                                     const std::vector<VarValue>& arguments)
{
  // TODO: make this non-latent
  return ExecuteSpSnippetAndGetPromise(
    GetName(), "GetCameraState", compatibilityPolicy, self, arguments, false,
    SpSnippetMode::kReturnResult, VarValue(-1));
}

void PapyrusGame::RaceMenuHelper(VarValue& self, const char* funcName,
                                 const std::vector<VarValue>& arguments)
{
  auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
  if (auto actor = compatibilityPolicy->GetDefaultActor(
        GetName(), funcName, self.GetMetaStackId())) {
    actor->SetRaceMenuOpen(true);
    SpSnippet(GetName(), funcName, serializedArgs)
      .Execute(actor, SpSnippetMode::kNoReturnResult);
  }
}

VarValue PapyrusGame::GetFormInternal(VarValue self,
                                      const std::vector<VarValue>& arguments,
                                      bool extended) const noexcept
{
  if (arguments.size() != 1) {
    return VarValue::None();
  }
  auto formId = static_cast<const int32_t>(arguments[0].CastToInt());
  constexpr const uint32_t maxId = 0x80000000;

  if (!extended && formId > maxId) {
    return VarValue::None();
  }

  const std::shared_ptr<MpForm>& pForm =
    compatibilityPolicy->GetWorldState()->LookupFormById(formId);
  espm::LookupResult res =
    compatibilityPolicy->GetWorldState()->GetEspm().GetBrowser().LookupById(
      formId);

  if (!pForm && !res.rec) {
    return VarValue::None();
  }

  return pForm ? VarValue(pForm->ToGameObject())
               : VarValue(std::make_shared<EspmGameObject>(res));
}

VarValue PapyrusGame::GetForm(
  VarValue self, const std::vector<VarValue>& arguments) const noexcept
{
  return GetFormInternal(self, arguments, false);
}

VarValue PapyrusGame::GetFormEx(
  VarValue self, const std::vector<VarValue>& arguments) const noexcept
{
  return GetFormInternal(self, arguments, true);
}

VarValue PapyrusGame::ShakeController(VarValue self,
                                      const std::vector<VarValue>& arguments)
{
  return ExecuteSpSnippetAndGetPromise(GetName(), "ShakeController",
                                       compatibilityPolicy, self, arguments);
}

void PapyrusGame::Register(VirtualMachine& vm,
                           std::shared_ptr<IPapyrusCompatibilityPolicy> policy)
{
  compatibilityPolicy = policy;

  AddStatic(vm, "IncrementStat", &PapyrusGame::IncrementStat);
  AddStatic(vm, "ForceThirdPerson", &PapyrusGame::ForceThirdPerson);
  AddStatic(vm, "DisablePlayerControls", &PapyrusGame::DisablePlayerControls);
  AddStatic(vm, "EnablePlayerControls", &PapyrusGame::EnablePlayerControls);
  AddStatic(vm, "FindClosestReferenceOfAnyTypeInListFromRef",
            &PapyrusGame::FindClosestReferenceOfAnyTypeInListFromRef);
  AddStatic(vm, "FindClosestReferenceOfTypeFromRef",
            &PapyrusGame::FindClosestReferenceOfTypeFromRef);
  AddStatic(vm, "GetPlayer", &PapyrusGame::GetPlayer);
  AddStatic(vm, "ShowRaceMenu", &PapyrusGame::ShowRaceMenu);
  AddStatic(vm, "ShowLimitedRaceMenu", &PapyrusGame::ShowLimitedRaceMenu);
  AddStatic(vm, "GetCameraState", &PapyrusGame::GetCameraState);
  AddStatic(vm, "GetForm", &PapyrusGame::GetForm);
  AddStatic(vm, "GetFormEx", &PapyrusGame::GetFormEx);
  AddStatic(vm, "ShakeController", &PapyrusGame::ShakeController);
}
