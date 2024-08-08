#pragma once
#include <string>

class GameModeEvent
{
public:
  virtual ~GameModeEvent() = default;
  virtual const char* GetName() const = 0;
  virtual std::string GetArgumentsJsonArray() const = 0;
};
