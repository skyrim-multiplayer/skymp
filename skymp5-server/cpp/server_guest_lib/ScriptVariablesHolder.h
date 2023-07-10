#pragma once
#include "libespm/Loader.h"
#include "papyrus-vm/CIString.h"
#include "papyrus-vm/VirtualMachine.h"
#include <functional>
#include <optional>
#include <unordered_map>

class EspmGameObject;
class WorldState;

class ScriptVariablesHolder : public IVariablesHolder
{
public:
  ScriptVariablesHolder(const std::string& myScriptName,
                        espm::LookupResult baseRecordWithScripts,
                        espm::LookupResult refrRecordWithScripts,
                        const espm::CombineBrowser* browser,
                        espm::CompressedFieldsCache* compressedFieldsCache,
                        WorldState* worldState);

  VarValue* GetVariableByName(const char* name, const PexScript& pex) override;

private:
  void FillProperties();
  void FillNormalVariables(const PexScript& pex);
  void FillState(const PexScript& pex);

  struct Script
  {
    espm::Script script;

    // To decode formIds for property values of Object type
    std::function<uint32_t(uint32_t rawId)> toGlobalId;
  };

  std::optional<Script> GetScript(const espm::LookupResult& lookupRes);

  struct ScriptsCache
  {
    std::unordered_map<uint32_t, std::shared_ptr<IGameObject>> objectsHolder;
  };

  static VarValue CastPrimitivePropertyValue(
    const espm::CombineBrowser& br, ScriptsCache& st,
    const espm::Property::Value& propValue, espm::PropertyType type,
    const std::function<uint32_t(uint32_t)>& toGlobalId,
    WorldState* worldState);

  static void CastProperty(const espm::CombineBrowser& br,
                           const espm::Property& prop, VarValue* out,
                           ScriptsCache* scriptsCache,
                           const std::function<uint32_t(uint32_t)>& toGlobalId,
                           WorldState* worldState);

  static espm::PropertyType GetElementType(espm::PropertyType arrayType);

  espm::LookupResult baseRecordWithScripts;
  espm::LookupResult refrRecordWithScripts;
  const std::string myScriptName;
  const espm::CombineBrowser* const browser;
  std::unique_ptr<CIMap<VarValue>> vars;
  VarValue state;
  std::unique_ptr<ScriptsCache> scriptsCache;
  espm::CompressedFieldsCache* const compressedFieldsCache;
  WorldState* const worldState;
};
