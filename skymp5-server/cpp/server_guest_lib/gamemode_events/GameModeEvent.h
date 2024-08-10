#pragma once
#include <string>
#include <utility>

struct VarValue;
class WorldState;

class GameModeEvent
{
public:
  virtual ~GameModeEvent() = default;
  virtual const char* GetName() const = 0;

  virtual std::string GetArgumentsJsonArray() const = 0;

  // Not all VarValues can be represented as JSON, so we need to pass them
  // separately. For example, a VarValue containing a Promise.
  virtual std::pair<const VarValue*, size_t> GetAdditionalArguments() const
  {
    return std::make_pair(nullptr, 0);
  }

  virtual std::string GetDetailedNameForLogging() const { return GetName(); }

  bool Fire(WorldState* worldState);

protected:
  virtual void OnFireBlocked(WorldState* worldState) { return; }
  virtual void OnFireSuccess(WorldState* worldState) = 0;
};
