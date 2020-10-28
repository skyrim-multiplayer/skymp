#include "ScriptVariablesHolder.h"

#include "EspmGameObject.h"
#include "Utils.h"

ScriptVariablesHolder::ScriptVariablesHolder(
  const std::string& myScriptName_, espm::RecordHeader* recordWithScripts_,
  const espm::CombineBrowser* browser_,
  espm::CompressedFieldsCache* compressedFieldsCache_)
  : recordWithScripts(recordWithScripts_)
  , myScriptName(myScriptName_)
  , browser(browser_)
  , compressedFieldsCache(compressedFieldsCache_)
{
}

VarValue* ScriptVariablesHolder::GetVariableByName(const char* name,
                                                   const PexScript& pex)
{
  if (!scriptsCache)
    scriptsCache.reset(new ScriptsCache);

  if (!vars) {
    vars.reset(new VarsMap);
    FillNormalVariables(pex);
    if (recordWithScripts)
      FillProperties(GetScript());
    FillState(pex);
  }

  auto it = vars->find(name);
  if (it != vars->end())
    return &it->second;
  return nullptr;
}

void ScriptVariablesHolder::FillProperties(const espm::Script& script)
{
  for (auto& prop : script.properties) {
    VarValue out;
    CastProperty(*browser, prop, &out, scriptsCache.get());
    CIString fullVarName;
    fullVarName += "::";
    fullVarName += prop.propertyName.data();
    fullVarName += "_var";
    (*vars)[fullVarName] = out;
  }
}

void ScriptVariablesHolder::FillNormalVariables(const PexScript& pex)
{
  for (auto& object : pex.objectTable.m_data) {
    for (auto& var : object.variables) {
      ObjectTable::Object::VarInfo varInfo;
      varInfo = var;
      if ((const char*)varInfo.value == nullptr) {
        varInfo.value =
          VarValue(ActivePexInstance::GetTypeByName(var.typeName));
      }
      (*vars)[CIString{ var.name.begin(), var.name.end() }] = varInfo.value;
    }
  }
}

void ScriptVariablesHolder::FillState(const PexScript& pex)
{
  // Creating temp variable for save State ActivePexInstance and
  // transition between them
  ObjectTable::Object::VarInfo variableForState = {
    "::State", "String", 0,
    VarValue(pex.objectTable.m_data[0].autoStateName.data())
  };
  (*vars)[CIString{ variableForState.name.begin(),
                    variableForState.name.end() }] = variableForState.value;
}

espm::Script ScriptVariablesHolder::GetScript()
{
  espm::ScriptData scriptData;
  recordWithScripts->GetScriptData(&scriptData, compressedFieldsCache);
  auto matchingScriptData = std::find_if(
    scriptData.scripts.begin(), scriptData.scripts.end(),
    [&](const espm::Script& script) {
      return !Utils::stricmp(script.scriptName.data(), myScriptName.data());
    });
  if (matchingScriptData == scriptData.scripts.end())
    throw std::runtime_error("ScriptData doesn't contain script with name '" +
                             myScriptName + "'");
  return std::move(*matchingScriptData);
}

VarValue ScriptVariablesHolder::CastPrimitivePropertyValue(
  const espm::CombineBrowser& br, ScriptsCache& st,
  const espm::Property::Value& propValue, espm::PropertyType type)
{
  switch (type) {
    case espm::PropertyType::Object: {
      if (!propValue.formId)
        return VarValue::None();
      auto& gameObject = st.espmObjectsHolder[propValue.formId];
      if (!gameObject) {
        auto formId = propValue.formId;
        auto lookupResult = br.LookupById(formId);
        if (!lookupResult.rec) {
          std::stringstream ss;
          ss << "CastPrimitivePropertyValue - Record with id " << std::hex
             << formId << " not found";
          throw std::runtime_error(ss.str());
        }
        gameObject.reset(new EspmGameObject(lookupResult));
      }
      return VarValue(gameObject.get());
    }
    case espm::PropertyType::String: {
      std::string str(propValue.str.data, propValue.str.length);
      auto& stringPtr = st.propStringValues[str];
      if (!stringPtr)
        stringPtr.reset(new std::string(str));
      return VarValue(stringPtr->data());
    }
    case espm::PropertyType::Int:
      return VarValue(propValue.integer);
    case espm::PropertyType::Float:
      return VarValue(propValue.floatingPoint);
    case espm::PropertyType::Bool:
      return VarValue(propValue.boolean);
    default:
      throw std::runtime_error(
        "Unexpected type in CastPrimitivePropertyValue (" +
        std::to_string(static_cast<int>(type)) + ")");
  }
}

void ScriptVariablesHolder::CastProperty(const espm::CombineBrowser& br,
                                         const espm::Property& prop,
                                         VarValue* out,
                                         ScriptsCache* scriptsCache)
{
  if (prop.propertyType >= espm::PropertyType::ObjectArray &&
      prop.propertyType <= espm::PropertyType::BoolArray) {
    VarValue v(static_cast<uint8_t>(prop.propertyType));
    v.pArray.reset(new std::vector<VarValue>);
    for (auto& entry : prop.array) {
      v.pArray->push_back(CastPrimitivePropertyValue(
        br, *scriptsCache, entry, GetElementType(prop.propertyType)));
    }
    *out = v;
    return;
  }

  *out = CastPrimitivePropertyValue(br, *scriptsCache, prop.value,
                                    prop.propertyType);
}

espm::PropertyType ScriptVariablesHolder::GetElementType(
  espm::PropertyType arrayType)
{
  return static_cast<espm::PropertyType>(static_cast<int>(arrayType) - 10);
}