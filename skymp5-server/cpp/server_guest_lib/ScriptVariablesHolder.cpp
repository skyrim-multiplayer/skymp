#include "ScriptVariablesHolder.h"

#include "WorldState.h"
#include "libespm/Property.h"
#include "papyrus-vm/Utils.h"
#include "script_objects/EspmGameObject.h"
#include "script_objects/MpFormGameObject.h"

#include <spdlog/spdlog.h>

ScriptVariablesHolder::ScriptVariablesHolder(
  const std::string& myScriptName_, espm::LookupResult baseRecordWithScripts_,
  espm::LookupResult refrRecordWithScripts_,
  const espm::CombineBrowser* browser_,
  espm::CompressedFieldsCache* compressedFieldsCache_, WorldState* worldState_)
  : baseRecordWithScripts(baseRecordWithScripts_)
  , refrRecordWithScripts(refrRecordWithScripts_)
  , myScriptName(myScriptName_)
  , browser(browser_)
  , compressedFieldsCache(compressedFieldsCache_)
  , worldState(worldState_)
{
}

VarValue* ScriptVariablesHolder::GetVariableByName(const char* name,
                                                   const PexScript& pex)
{
  if (!scriptsCache) {
    scriptsCache = std::make_unique<ScriptsCache>();
  }

  if (!Utils::stricmp(name, "::State")) {
    if (state == VarValue::None()) {
      FillState(pex);
    }
    return &state;
  }

  if (!vars) {
    vars = std::make_unique<CIMap<VarValue>>();
    FillNormalVariables(pex);
    FillProperties();
  }

  auto it = vars->find(name);
  if (it != vars->end()) {
    return &it->second;
  }
  return nullptr;
}

void ScriptVariablesHolder::FillProperties()
{
  auto baseScript = GetScript(baseRecordWithScripts);
  auto refrScript = GetScript(refrRecordWithScripts);

  for (auto& script : { baseScript, refrScript }) {
    if (script) {
      for (auto& prop : script->script.properties) {
        VarValue out;
        CastProperty(*browser, prop, &out, scriptsCache.get(),
                     script->toGlobalId, worldState);
        CIString fullVarName;
        fullVarName += "::";
        fullVarName += prop.name.data();
        fullVarName += "_var";
        (*vars)[fullVarName] = out;

        if (spdlog::should_log(spdlog::level::trace)) {
          spdlog::trace(
            "FillProperties for script {}: Adding property {} with value {}",
            script->script.scriptName, fullVarName.data(), out.ToString());
        }
      }
    }
  }
}

void ScriptVariablesHolder::FillNormalVariables(const PexScript& pex)
{
  for (auto& object : pex.objectTable) {
    for (auto& var : object.variables) {
      Object::VarInfo varInfo;
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
  state = VarValue(pex.objectTable[0].autoStateName.data());
}

std::optional<ScriptVariablesHolder::Script> ScriptVariablesHolder::GetScript(
  const espm::LookupResult& lookupRes)
{
  if (!lookupRes.rec) {
    return std::nullopt;
  }

  espm::ScriptData scriptData;
  lookupRes.rec->GetScriptData(&scriptData, *compressedFieldsCache);
  auto matchingScriptData = std::find_if(
    scriptData.scripts.begin(), scriptData.scripts.end(),
    [&](const espm::Script& script) {
      return !Utils::stricmp(script.scriptName.data(), myScriptName.data());
    });
  if (matchingScriptData != scriptData.scripts.end()) {
    ScriptVariablesHolder::Script result;
    result.script = std::move(*matchingScriptData);
    result.toGlobalId = [lookupRes](uint32_t rawId) {
      return lookupRes.ToGlobalId(rawId);
    };
    return result;
  }
  return std::nullopt;
}

VarValue ScriptVariablesHolder::CastPrimitivePropertyValue(
  const espm::CombineBrowser& br, ScriptsCache& st,
  const espm::Property::Value& propValue, espm::Property::Type type,
  const std::function<uint32_t(uint32_t)>& toGlobalId, WorldState* worldState)
{
  switch (type) {
    case espm::Property::Type::Object: {
      if (!propValue.formId) {
        return VarValue::None();
      }
      auto propValueFormIdGlobal = toGlobalId(propValue.formId);
      spdlog::trace(
        "CastPrimitivePropertyValue {} - Prop to global id {:x} -> {:x}",
        myScriptName, propValue.formId, propValueFormIdGlobal);
      auto& gameObject = st.objectsHolder[propValueFormIdGlobal];
      if (!gameObject) {
        auto lookupResult = br.LookupById(propValueFormIdGlobal);
        if (!lookupResult.rec) {
          spdlog::error(
            "CastPrimitivePropertyValue {} - Record with id {:x} not found",
            myScriptName, propValueFormIdGlobal);
        } else {
          auto type = lookupResult.rec->GetType();
          if (type == espm::REFR::kType || type == espm::ACHR::kType) {
            if (worldState) {
              std::stringstream traceStream;
              auto& form = worldState->LookupFormById(propValueFormIdGlobal,
                                                      &traceStream);
              if (form != nullptr) {
                gameObject = std::make_shared<MpFormGameObject>(form.get());
                spdlog::trace("CastPrimitivePropertyValue {} - Created {} "
                              "(MpFormGameObject) property with id {:x}",
                              myScriptName, type.ToString(),
                              propValueFormIdGlobal);
              } else {
                spdlog::warn(
                  "CastPrimitivePropertyValue {} - Unable to create {} "
                  "(MpFormGameObject) property with id {:x}, form "
                  "not found in the world. LookupFormById trace:\n{}",
                  myScriptName, type.ToString(), propValueFormIdGlobal,
                  traceStream.str());
              }
            } else {
              spdlog::error(
                "CastPrimitivePropertyValue {} - Unable to create {} "
                "(MpFormGameObject) property with id {:x}, null "
                "WorldState",
                myScriptName, type.ToString(), propValueFormIdGlobal);
            }
          } else {
            gameObject = std::make_shared<EspmGameObject>(lookupResult);
            spdlog::trace("CastPrimitivePropertyValue {} - Created {} "
                          "(EspmGameObject) property with id {:x}",
                          myScriptName, type.ToString(),
                          propValueFormIdGlobal);
          }
        }
      }
      return VarValue(gameObject.get());
    }
    case espm::Property::Type::String: {
      std::string str(propValue.str.data, propValue.str.length);
      return VarValue(str);
    }
    case espm::Property::Type::Int:
      return VarValue(propValue.integer);
    case espm::Property::Type::Float:
      return VarValue(propValue.floatingPoint);
    case espm::Property::Type::Bool:
      return VarValue(propValue.boolean);
    default:
      spdlog::error("Unexpected type in CastPrimitivePropertyValue ({})",
                    static_cast<int>(type));
      return VarValue::None();
  }
}

void ScriptVariablesHolder::CastProperty(
  const espm::CombineBrowser& br, const espm::Property& prop, VarValue* out,
  ScriptsCache* scriptsCache,
  const std::function<uint32_t(uint32_t)>& toGlobalId, WorldState* worldState)
{
  if (prop.type >= espm::Property::Type::ObjectArray &&
      prop.type <= espm::Property::Type::BoolArray) {
    VarValue v(static_cast<uint8_t>(prop.type));
    v.pArray.reset(new std::vector<VarValue>);
    for (auto& entry : prop.array) {
      v.pArray->push_back(CastPrimitivePropertyValue(br, *scriptsCache, entry,
                                                     GetElementType(prop.type),
                                                     toGlobalId, worldState));
    }
    *out = v;
    return;
  }

  *out = CastPrimitivePropertyValue(br, *scriptsCache, prop.value, prop.type,
                                    toGlobalId, worldState);
}

espm::Property::Type ScriptVariablesHolder::GetElementType(
  espm::Property::Type arrayType)
{
  return static_cast<espm::Property::Type>(
    static_cast<std::underlying_type_t<espm::Property::Type>>(arrayType) - 10);
}
