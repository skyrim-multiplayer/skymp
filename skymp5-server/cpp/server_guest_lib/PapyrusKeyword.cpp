#include "PapyrusKeyword.h"

VarValue PapyrusKeyword::GetKeyword(VarValue self,
                                    const std::vector<VarValue>& arguments)
{
  if (arguments.empty()) {
    spdlog::error("Form.HasKeyword - at least one argument expected");
    return VarValue();
  }

  std::string keywordName = arguments[0].ToString();

  for (size_t i = 0; i < keywords.size(); ++i) {
    auto& espmLocalKeywords = keywords[i];
    auto it = std::find_if(
      espmLocalKeywords->begin(), espmLocalKeywords->end(),
      [&](espm::RecordHeader* rec) {
        auto keyword = reinterpret_cast<espm::KYWD*>(rec);
        return keyword->GetData(worldState->GetEspmCache()).editorId ==
          keywordName;
      });
    if (it != espmLocalKeywords->end()) {
      espm::KYWD* keywordUsed = reinterpret_cast<espm::KYWD*>(*it);
      return VarValue(std::make_shared<EspmGameObject>(
        worldState->GetEspm().GetBrowser().LookupById(keywordUsed->GetId())));
    }
  }

  return VarValue();
}
