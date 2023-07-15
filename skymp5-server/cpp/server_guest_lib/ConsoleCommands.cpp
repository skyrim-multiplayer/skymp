#include "ConsoleCommands.h"
#include "EspmGameObject.h"
#include "MpActor.h"
#include "PapyrusObjectReference.h"
#include "WorldState.h"
#include "papyrus-vm/Utils.h"

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

namespace {

void EnsureAdmin(const MpActor& me)
{
  bool isAdmin = me.GetConsoleCommandsAllowedFlag();
  if (!isAdmin) {
    throw std::runtime_error("Not enough permissions to use this command");
  }
}

void ExecuteAddItem(MpActor& caller,
                    const std::vector<ConsoleCommands::Argument>& args)
{
  EnsureAdmin(caller);

  const auto targetId = static_cast<uint32_t>(args.at(0).GetInteger());
  const auto itemId = static_cast<uint32_t>(args.at(1).GetInteger());
  const auto count = static_cast<int32_t>(args.at(2).GetInteger());

  MpObjectReference& target = (targetId == 0x14)
    ? caller
    : caller.GetParent()->GetFormAt<MpObjectReference>(targetId);

  auto& br = caller.GetParent()->GetEspm().GetBrowser();

  PapyrusObjectReference papyrusObjectReference;
  auto aItem =
    VarValue(std::make_shared<EspmGameObject>(br.LookupById(itemId)));
  auto aCount = VarValue(count);
  auto aSilent = VarValue(false);
  (void)papyrusObjectReference.AddItem(target.ToVarValue(),
                                       { aItem, aCount, aSilent });
}

void ExecutePlaceAtMe(MpActor& caller,
                      const std::vector<ConsoleCommands::Argument>& args)
{
  EnsureAdmin(caller);

  const auto targetId = static_cast<uint32_t>(args.at(0).GetInteger());
  const auto baseFormId = static_cast<uint32_t>(args.at(1).GetInteger());

  MpObjectReference& target = (targetId == 0x14)
    ? caller
    : caller.GetParent()->GetFormAt<MpObjectReference>(targetId);

  auto& br = caller.GetParent()->GetEspm().GetBrowser();

  PapyrusObjectReference papyrusObjectReference;
  auto aBaseForm =
    VarValue(std::make_shared<EspmGameObject>(br.LookupById(baseFormId)));
  auto aCount = VarValue(1);
  auto aForcePersist = VarValue(false);
  auto aInitiallyDisabled = VarValue(false);
  (void)papyrusObjectReference.PlaceAtMe(
    target.ToVarValue(),
    { aBaseForm, aCount, aForcePersist, aInitiallyDisabled });
}

void ExecuteDisable(MpActor& caller,
                    const std::vector<ConsoleCommands::Argument>& args)
{
  EnsureAdmin(caller);

  const auto targetId = static_cast<uint32_t>(args.at(0).GetInteger());

  MpObjectReference& target = (targetId == 0x14)
    ? caller
    : caller.GetParent()->GetFormAt<MpObjectReference>(targetId);

  if (target.GetFormId() >= 0xff000000)
    target.Disable();
}

void ExecuteMp(MpActor& caller,
               const std::vector<ConsoleCommands::Argument>& args)
{
  auto subcmd = args.at(1).GetString();
  if (!Utils::stricmp(subcmd.data(), "disable")) {
    return ExecuteDisable(caller, args);
  }
}
}

void ConsoleCommands::Execute(
  MpActor& me, const std::string& consoleCommandName,
  const std::vector<ConsoleCommands::Argument>& args)
{
  if (!Utils::stricmp(consoleCommandName.data(), "AddItem")) {
    ExecuteAddItem(me, args);
  } else if (!Utils::stricmp(consoleCommandName.data(), "PlaceAtMe")) {
    ExecutePlaceAtMe(me, args);
  } else if (!Utils::stricmp(consoleCommandName.data(), "Disable")) {
    ExecuteDisable(me, args);
  } else if (!Utils::stricmp(consoleCommandName.data(), "Mp")) {
    ExecuteMp(me, args);
  } else {
    throw std::runtime_error("Unknown command name '" + consoleCommandName +
                             "'");
  }
}
