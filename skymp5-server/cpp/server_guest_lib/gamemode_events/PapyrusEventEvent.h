#pragma once
#include "GameModeEvent.h"

class MpForm;
class VarValue;

class PapyrusEventEvent : public GameModeEvent
{
public:
  PapyrusEventEvent(MpForm* form_, const char* papyrusEventName_,
                    const VarValue* arguments_, size_t numArguments_);

  const char* GetName() const override;

  std::string GetArgumentsJsonArray() const override;

  std::pair<const VarValue*, size_t> GetAdditionalArguments() const override;

  std::string GetDetailedNameForLogging() const override;

private:
  void OnFireSuccess(WorldState*) override;

  MpForm* form = nullptr;
  const char* papyrusEventName = "";
  const VarValue* arguments = nullptr;
  size_t numArguments = 0;
  bool papyrusEventNameEscape = false;
};
