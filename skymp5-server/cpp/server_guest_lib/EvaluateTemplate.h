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

  std::stringstream detailedLog;

  for (auto it = chain.begin(); it != chain.end(); it++) {
    auto templateChainElement = it->ToFormId(worldState->espmFiles);

    detailedLog << "Processing FormDesc: " << it->ToString() << "\n";

    auto npcLookupResult =
      worldState->GetEspm().GetBrowser().LookupById(templateChainElement);
    detailedLog << "Variable npcLookupResult: " << npcLookupResult.rec << "\n";

    auto npc = espm::Convert<espm::NPC_>(npcLookupResult.rec);
    detailedLog << "Variable npc: " << npc << "\n";

    if (!npc) {
      detailedLog << "Variable npc was nullptr, failing EvaluateTemplate\n";
      break;
    }

    auto npcData = npc->GetData(worldState->GetEspmCache());

    detailedLog << "Variable npcData: baseTemplate=" << npcData.baseTemplate
                << ", templateDataFlags=" << npcData.templateDataFlags << "\n";

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
  ss << ", detailedLog=" << detailedLog.str();

  throw std::runtime_error(ss.str());
}

template <uint16_t TemplateFlag, class Callback>
auto EvaluateTemplateNoThrow(WorldState* worldState, uint32_t baseId,
                             const std::vector<FormDesc>& templateChain,
                             const Callback& callback,
                             std::string* outException)
{
  using ResultType = decltype(EvaluateTemplate<TemplateFlag, Callback>(
    worldState, baseId, templateChain, callback));
  std::optional<ResultType> result;

  try {
    result = EvaluateTemplate<TemplateFlag, Callback>(worldState, baseId,
                                                      templateChain, callback);
  } catch (std::exception& e) {
    if (outException) {
      *outException = e.what();
    }
    result.reset();
  }

  return result;
}
