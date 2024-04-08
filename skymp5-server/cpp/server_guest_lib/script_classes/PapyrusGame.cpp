#include "PapyrusGame.h"
#include "PapyrusFormList.h"

#include "WorldState.h"
#include "libespm/Combiner.h"
#include "script_objects/EspmGameObject.h"
#include "script_objects/MpFormGameObject.h"

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

VarValue PapyrusGame::FindClosestReferenceOfAnyTypeInListFromRef(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() >= 3) {
    auto arBaseObjects = arguments[0];
    auto arCenter = GetFormPtr<MpObjectReference>(arguments[1]);
    double afRadius = static_cast<double>(arguments[2].CastToFloat());

    if (arBaseObjects && arCenter && afRadius >= 0) {

      float bestDistance = std::numeric_limits<float>::infinity();
      MpObjectReference* bestNeighbour = nullptr;

      arCenter->VisitNeighbours([&](MpObjectReference* neighbour) {
        auto baseId = neighbour->GetBaseId();
        if (!ExistsInFormList(arBaseObjects, baseId))
          return;

        float distance = (arCenter->GetPos() - neighbour->GetPos()).Length();
        if (distance > afRadius)
          return;

        if (bestDistance > distance) {
          bestDistance = distance;
          bestNeighbour = neighbour;
        }
      });

      if (bestNeighbour)
        return VarValue(std::make_shared<MpFormGameObject>(bestNeighbour));
    }
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
  auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
  if (auto actor = compatibilityPolicy->GetDefaultActor(
        GetName(), "GetCameraState", self.GetMetaStackId())) {
    Viet::Promise<VarValue> promise =
      SpSnippet(GetName(), "GetCameraState", serializedArgs.data())
        .Execute(actor);
    return VarValue(Viet::Promise<VarValue>(promise));
  }
  return VarValue(-1);
}

void PapyrusGame::RaceMenuHelper(VarValue& self, const char* funcName,
                                 const std::vector<VarValue>& arguments)
{
  auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
  if (auto actor = compatibilityPolicy->GetDefaultActor(
        GetName(), funcName, self.GetMetaStackId())) {
    actor->SetRaceMenuOpen(true);
    SpSnippet(GetName(), funcName, serializedArgs.data()).Execute(actor);
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
  espm::LookupResult res = GetRecordPtr(VarValue(formId));

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
  auto funcName = "ShakeController";

  auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
  if (auto actor = compatibilityPolicy->GetDefaultActor(
        GetName(), funcName, self.GetMetaStackId())) {
    SpSnippet(GetName(), funcName, serializedArgs.data()).Execute(actor);
  }

  return VarValue::None();
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
  AddStatic(vm, "GetPlayer", &PapyrusGame::GetPlayer);
  AddStatic(vm, "ShowRaceMenu", &PapyrusGame::ShowRaceMenu);
  AddStatic(vm, "ShowLimitedRaceMenu", &PapyrusGame::ShowLimitedRaceMenu);
  AddStatic(vm, "GetCameraState", &PapyrusGame::GetCameraState);
  AddStatic(vm, "GetForm", &PapyrusGame::GetForm);
  AddStatic(vm, "GetFormEx", &PapyrusGame::GetFormEx);
  AddStatic(vm, "ShakeController", &PapyrusGame::ShakeController);
}
