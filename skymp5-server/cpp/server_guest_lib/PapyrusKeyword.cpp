#include "PapyrusKeyword.h"

VarValue PapyrusKeyword::GetKeyword(VarValue self,
                                    const std::vector<VarValue>& arguments)
{
  if (arguments.empty()) {
    spdlog::error("Keyword.GetKeyword - at least one argument expected");
    return VarValue::None();
  }

  CIString keywordName = arguments[0].GetType() == VarValue::kType_String
    ? static_cast<const char*>(arguments[0])
    : "";

  for (auto it = keywords.rbegin(); it != keywords.rend(); ++it) {
    for (auto itKeyword = (*it)->begin(); itKeyword != (*it)->end();
         ++itKeyword) {
      auto keyword = reinterpret_cast<espm::KYWD*>(*itKeyword);
      CIString otherName =
        keyword->GetData(worldState->GetEspmCache()).editorId;

      if (otherName == keywordName) {
        return VarValue(std::make_shared<EspmGameObject>(
          worldState->GetEspm().GetBrowser().LookupById(keyword->GetId())));
      }
    }
  }

  return VarValue::None();
}
