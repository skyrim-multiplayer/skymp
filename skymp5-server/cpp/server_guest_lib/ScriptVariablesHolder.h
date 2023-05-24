#pragma once
#include "CIString.h"
#include "Loader.h"
#include "VirtualMachine.h"
#include <optional>

class EspmGameObject;

class ScriptVariablesHolder : public IVariablesHolder
{
public:
  ScriptVariablesHolder(const std::string& myScriptName,
                        espm::RecordHeader* baseRecordWithScripts,
                        espm::RecordHeader* refrRecordWithScripts,
                        const espm::CombineBrowser* browser,
                        espm::CompressedFieldsCache* compressedFieldsCache);

  VarValue* GetVariableByName(const char* name, const PexScript& pex) override;

private:
  void FillProperties();
  void FillNormalVariables(const PexScript& pex);
  void FillState(const PexScript& pex);
  
  std::optional<espm::Script> GetScript(espm::RecordHeader *const record);

  using VarsMap = CIMap<VarValue>;
  using EspmObjectsHolder =
    std::map<uint32_t, std::shared_ptr<EspmGameObject>>;
  using PropStringValues = std::map<std::string, std::shared_ptr<std::string>>;
  struct ScriptsCache
  {
    EspmObjectsHolder espmObjectsHolder;
    PropStringValues propStringValues;
  };

  static VarValue CastPrimitivePropertyValue(
    const espm::CombineBrowser& br, ScriptsCache& st,
    const espm::Property::Value& propValue, espm::PropertyType type);

  static void CastProperty(const espm::CombineBrowser& br,
                           const espm::Property& prop, VarValue* out,
                           ScriptsCache* scriptsCache);
  static espm::PropertyType GetElementType(espm::PropertyType arrayType);

  espm::RecordHeader* const baseRecordWithScripts;
  espm::RecordHeader* const refrRecordWithScripts;
  const std::string myScriptName;
  const espm::CombineBrowser* const browser;
  std::unique_ptr<VarsMap> vars;
  VarValue state;
  std::unique_ptr<ScriptsCache> scriptsCache;
  espm::CompressedFieldsCache* const compressedFieldsCache;
};
