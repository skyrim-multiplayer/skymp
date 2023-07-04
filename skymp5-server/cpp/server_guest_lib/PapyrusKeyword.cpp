#include "PapyrusUtility.h"

#include "WorldState.h"
#include "PapyrusKeyword.h"
#include "EspmGameObject.h"

extern espm::Loader L;

VarValue PapyrusKeyword::GetKeyword(VarValue self,
                                    const std::vector<VarValue>& arguments)
{
  static espm::CompressedFieldsCache g_dummyCache;

  if (arguments.empty()) {
    spdlog::error("Form.HasKeyword - at least one argument expected");
    return VarValue();
  }

  const auto& keywordRec = GetRecordPtr(arguments[0]);
  if (!keywordRec.rec) {
    spdlog::error("Form.HasKeyword - invalid keyword form");
    return VarValue();
  }

  L.GetBrowser().GetRecordsByType("KYWD");
  return VarValue(keywordRec.rec);
}

const espm::LookupResult& GetRecordPtr(const VarValue& papyrusObject)
{
  static const espm::LookupResult emptyResult;

  if (papyrusObject.GetType() != VarValue::kType_Object) {
    std::stringstream papyrusObjectStr;
    papyrusObjectStr << papyrusObject;
    spdlog::warn("GetRecordPtr called with non-object ({})",
                 papyrusObjectStr.str());
    return emptyResult;
  }
  auto gameObject = static_cast<IGameObject*>(papyrusObject);
  if (!gameObject) {
    std::stringstream papyrusObjectStr;
    papyrusObjectStr << papyrusObject;
    spdlog::warn("GetRecordPtr called with null object ({})",
                 papyrusObjectStr.str());
    return emptyResult;
  }
  auto espmGameObject = dynamic_cast<EspmGameObject*>(gameObject);
  if (!espmGameObject) {
    std::stringstream papyrusObjectStr;
    papyrusObjectStr << papyrusObject;
    spdlog::warn("GetRecordPtr called with non-espm object ({})",
                 papyrusObjectStr.str());
    return emptyResult;
  }
  return espmGameObject->record;
}
