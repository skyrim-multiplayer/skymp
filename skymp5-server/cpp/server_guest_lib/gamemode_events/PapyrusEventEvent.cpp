#include "PapyrusEventEvent.h"

#include "MpForm.h"
#include "WorldState.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string_view>

constexpr auto kSafeJsonStringAlphabet =
  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";

PapyrusEventEvent::PapyrusEventEvent(MpForm* form_,
                                     const char* papyrusEventName_,
                                     const VarValue* arguments_,
                                     size_t numArguments_)
  : form(form_)
  , papyrusEventName(papyrusEventName_)
  , arguments(arguments_)
  , numArguments(numArguments_)
  , papyrusEventNameEscape(std::string_view(papyrusEventName_)
                             .find_first_not_of(kSafeJsonStringAlphabet) !=
                           std::string_view::npos)
{
}

const char* PapyrusEventEvent::GetName() const
{
  return "papyrusEvent";
}

std::string PapyrusEventEvent::GetArgumentsJsonArray() const
{
  std::string result;
  result += "[";
  result += std::to_string(form->GetFormId());
  result += ",";

  if (papyrusEventNameEscape) {
    result += nlohmann::json(papyrusEventName).dump();
  } else {
    result += "\"";
    result += papyrusEventName;
    result += "\"";
  }

  result += "]";
  return result;
}

std::pair<const VarValue*, size_t> PapyrusEventEvent::GetAdditionalArguments()
  const
{
  return std::make_pair(arguments, numArguments);
}

std::string PapyrusEventEvent::GetDetailedNameForLogging() const
{
  std::string result;
  result += GetName();
  result += ':';
  result += papyrusEventName;
  return result;
}

void PapyrusEventEvent::OnFireSuccess(WorldState* worldState)
{
  worldState->SendPapyrusEvent(form, papyrusEventName, arguments,
                               numArguments);
}
