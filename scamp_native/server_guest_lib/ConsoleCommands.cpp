#include "ConsoleCommands.h"

ConsoleCommands::Argument::Argument()
{
  data = 0;
}

ConsoleCommands::Argument::Argument(int64_t integer)
{
  data = integer;
}

ConsoleCommands::Argument::Argument(const std::string& str)
{
  data = str;
}

bool ConsoleCommands::Argument::IsInteger() const noexcept
{
  return data.index() == 0;
}
bool ConsoleCommands::Argument::IsString() const noexcept
{
  return data.index() == 1;
};

int64_t ConsoleCommands::Argument::GetInteger() const
{
  if (!IsInteger())
    throw std::runtime_error(
      "ConsoleCommands::Argument - Expected to be Integer");
  return std::get<int64_t>(data);
}

const std::string& ConsoleCommands::Argument::GetString() const
{
  if (!IsString())
    throw std::runtime_error(
      "ConsoleCommands::Argument - Expected to be String");
  return std::get<std::string>(data);
}