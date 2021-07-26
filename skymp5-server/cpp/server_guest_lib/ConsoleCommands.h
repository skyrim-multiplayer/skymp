#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

class MpActor;

namespace ConsoleCommands {
class Argument
{
public:
  Argument();
  Argument(int64_t integer);
  Argument(const std::string& str);

  bool IsInteger() const noexcept;
  bool IsString() const noexcept;
  int64_t GetInteger() const;
  const std::string& GetString() const;

  friend bool operator==(const Argument& lhs, const Argument& rhs)
  {
    return lhs.data == rhs.data;
  }

  friend bool operator!=(const Argument& lhs, const Argument& rhs)
  {
    return lhs.data != rhs.data;
  }

private:
  std::variant<int64_t, std::string> data;
};

void Execute(MpActor& me, const std::string& consoleCommandName,
             const std::vector<ConsoleCommands::Argument>& args);
}
