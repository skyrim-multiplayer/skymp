#include "PapyrusEventEvent.h"

#include "MpForm.h"
#include "WorldState.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string_view>

PapyrusEventEvent::PapyrusEventEvent(MpForm* form_,
                                     const char* papyrusEventName_,
                                     const VarValue* arguments_,
                                     size_t numArguments_)
  : form(form_)
  , papyrusEventName(papyrusEventName_)
  , arguments(arguments_)
  , numArguments(numArguments_)
{
  eventNameFull += "onPapyrusEvent:";
  eventNameFull += papyrusEventName;
}

const char* PapyrusEventEvent::GetName() const
{
  return eventNameFull.data();
}

std::string PapyrusEventEvent::GetArgumentsJsonArray() const
{
  std::string result;
  result += "[";
  result += std::to_string(form->GetFormId());
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
  return eventNameFull;
}

void PapyrusEventEvent::OnFireSuccess(WorldState* worldState)
{
  worldState->SendPapyrusEvent(form, papyrusEventName, arguments,
                               numArguments);
}
