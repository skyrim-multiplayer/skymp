#include "IsInInterior.h"
#include "MpActor.h"
#include "MpObjectReference.h"

const char* ConditionFunctions::IsInInterior::GetName() const
{
  return "IsInInterior";
}

uint16_t ConditionFunctions::IsInInterior::GetFunctionIndex() const
{
  return 300;
}

float ConditionFunctions::IsInInterior::Execute(
  MpActor& actor, [[maybe_unused]] uint32_t parameter1,
  [[maybe_unused]] uint32_t parameter2, const ConditionEvaluatorContext&)
{
  const FormDesc& cellOrWorld = actor.GetCellOrWorld();

  auto& browser = actor.GetParent()->GetEspm().GetBrowser();

  espm::LookupResult lookupRes =
    browser.LookupById(cellOrWorld.ToFormId(actor.GetParent()->espmFiles));

  // In SkyMP, GetCellOrWorld never returns exterior cells, only interior cells
  // or worldspaces
  if (lookupRes.rec &&
      lookupRes.rec->GetType().ToString() == espm::CELL::kType) {
    return 1.f;
  }

  return 0.f;
}
