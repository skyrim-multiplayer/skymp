#include "PapyrusGame.h"
#include "PapyrusFormList.h"

#include "EspmGameObject.h"
#include "MpFormGameObject.h"
#include "WorldState.h"

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
