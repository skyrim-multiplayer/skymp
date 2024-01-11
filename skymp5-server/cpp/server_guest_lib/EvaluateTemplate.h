#pragma once
#include "FormDesc.h"
#include "WorldState.h"
#include "libespm/espm.h"
#include <sstream>
#include <stdexcept>
#include <vector>

template <uint16_t TemplateFlag, class Callback>
auto EvaluateTemplate(WorldState* worldState, uint32_t baseId,
                      const std::vector<FormDesc>& templateChain,
                      const Callback& callback)
{
  const std::vector<FormDesc> chainDefault = { FormDesc::FromFormId(
    baseId, worldState->espmFiles) };
  const std::vector<FormDesc>& chain =
    templateChain.size() > 0 ? templateChain : chainDefault;

  for (auto it = chain.begin(); it != chain.end(); it++) {
    auto templateChainElement = it->ToFormId(worldState->espmFiles);
    auto npcLookupResult =
      worldState->GetEspm().GetBrowser().LookupById(templateChainElement);
    auto npc = espm::Convert<espm::NPC_>(npcLookupResult.rec);
    auto npcData = npc->GetData(worldState->GetEspmCache());

    if (npcData.baseTemplate == 0) {
      return callback(npcLookupResult, npcData);
    }

    if (!(npcData.templateDataFlags & TemplateFlag)) {
      return callback(npcLookupResult, npcData);
    }
  }

  std::stringstream ss;
  ss << "EvaluateTemplate failed: baseId=" << std::hex << baseId
     << ", templateChain=";

  for (size_t i = 0; i < templateChain.size(); ++i) {
    ss << templateChain[i].ToString();
    if (i != templateChain.size() - 1) {
      ss << ",";
    }
  }

  ss << ", templateFlag=" << TemplateFlag;

  throw std::runtime_error(ss.str());
}
