#include "SpSnippetFunctionGen.h"

#include "SpSnippet.h"
#include "WorldState.h"
#include "script_objects/EspmGameObject.h"
#include "script_objects/MpFormGameObject.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

uint32_t SpSnippetFunctionGen::GetFormId(const VarValue& varValue)
{
  if (varValue == VarValue::None()) {
    return 0;
  }

  if (auto form = GetFormPtr<MpForm>(varValue)) {
    return form->GetFormId();
  }

  if (auto record = GetRecordPtr(varValue); record.rec) {
    return record.ToGlobalId(record.rec->GetId());
  }

  spdlog::error("SpSnippetFunctionGen::GetFormId - VarValue {} is not a valid "
                "Papyrus object",
                varValue.ToString());

  return 0;
}

std::vector<std::optional<
  std::variant<bool, double, std::string, SpSnippetObjectArgument>>>
SpSnippetFunctionGen::SerializeArguments(
  const std::vector<VarValue>& arguments, MpActor* actor)
{
  std::vector<std::optional<
    std::variant<bool, double, std::string, SpSnippetObjectArgument>>>
    result;
  result.reserve(arguments.size());

  for (auto& arg : arguments) {
    switch (arg.GetType()) {
      case VarValue::kType_Object: {
        auto formId = GetFormId(arg);

        // Player character is always 0x14 on client, but 0xff000000+ in our
        // server
        // See also SpSnippet.cpp
        if (actor && formId >= 0xff000000) {
          formId = formId != actor->GetFormId() ? formId : 0x14;
        }

        auto obj = static_cast<IGameObject*>(arg);
        auto type = obj ? obj->GetParentNativeScript() : "";

        SpSnippetObjectArgument objectArg;
        objectArg.formId = formId;
        objectArg.type = type;
        result.push_back(objectArg);
        break;
      }
      case VarValue::kType_String:
        result.push_back(std::string(static_cast<const char*>(arg)));
        break;
      case VarValue::kType_Bool:
        result.push_back(static_cast<bool>(arg));
        break;
      case VarValue::kType_Integer:
        result.push_back(static_cast<double>(static_cast<int>(arg)));
        break;
      case VarValue::kType_Float:
        result.push_back(static_cast<double>(arg));
        break;
      default: {
        spdlog::error("SpSnippetFunctionGen::SerializeArguments - Unable to "
                      "serialize VarValue {} due to unsupported type ({})",
                      arg.ToString(), static_cast<int>(arg.GetType()));
        result.push_back(std::nullopt);
        break;
      }
    }
  }
  return result;
}
