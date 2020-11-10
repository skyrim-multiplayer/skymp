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
    float afRadius = static_cast<float>(arguments[2].CastToFloat());

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