#pragma once
#include "CIString.h"
#include "Loader.h"
#include "VirtualMachine.h"
#include <optional>
#include <functional>

class EspmGameObject;

class ScriptVariablesHolder : public IVariablesHolder
{
public:
  ScriptVariablesHolder(const std::string& myScriptName,
                        espm::LookupResult baseRecordWithScripts,
                        espm::LookupResult refrRecordWithScripts,
                        const espm::CombineBrowser* browser,
                        espm::CompressedFieldsCache* compressedFieldsCache);

  VarValue* GetVariableByName(const char* name, const PexScript& pex) override;

private:
  void FillProperties();
  void FillNormalVariables(const PexScript& pex);
  void FillState(const PexScript& pex);

  struct Script {
    espm::Script script;

    // To decode formIds for property values of Object type
    std::function<uint32_t(uint32_t rawId)> toGlobalId;
  };

  std::optional<Script> GetScript(const espm::LookupResult &lookupRes);

  using VarsMap = CIMap<VarValue>;
  using EspmObjectsHolder =
    std::map<uint32_t, std::shared_ptr<EspmGameObject>>;
  struct ScriptsCache
  {
    EspmObjectsHolder espmObjectsHolder;
  };

  static VarValue CastPrimitivePropertyValue(
    const espm::CombineBrowser& br, ScriptsCache& st,
    const espm::Property::Value& propValue, espm::PropertyType type, 
    const std::function<uint32_t(uint32_t)> &toGlobalId);

  static void CastProperty(const espm::CombineBrowser& br,
                           const espm::Property& prop, VarValue* out,
                           ScriptsCache* scriptsCache, 
                           const std::function<uint32_t(uint32_t)> &toGlobalId);
  static espm::PropertyType GetElementType(espm::PropertyType arrayType);

  espm::LookupResult baseRecordWithScripts;
  espm::LookupResult refrRecordWithScripts;
  const std::string myScriptName;
  const espm::CombineBrowser* const browser;
  std::unique_ptr<VarsMap> vars;
  VarValue state;
  std::unique_ptr<ScriptsCache> scriptsCache;
  espm::CompressedFieldsCache* const compressedFieldsCache;
};
