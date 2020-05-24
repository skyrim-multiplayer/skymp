#include "ConsoleApi.h"
#include "NullPointerException.h"
#include <RE/CommandTable.h>
#include <RE/ConsoleLog.h>
#include <RE/Script.h>
#include <RE/TESForm.h>
#include <RE/TESObjectREFR.h>
#include <cstdlib>
#include <ctpl/ctpl_stl.h>
#include <map>
#include <skse64/ObScript.h>
#include <skse64_common/SafeWrite.h>
#include <vector>

extern ctpl::thread_pool g_pool;
extern TaskQueue g_taskQueue;

namespace {
struct ConsoleComand
{
  std::string longName;
  std::string shortName;
  uint16_t numArgs = 0;
  ObScript_Execute execute;
  JsValue jsExecute;
  ObScriptCommand* myIter;
  ObScriptCommand myOriginalData;
};
static std::map<std::string, ConsoleComand> replacedConsoleCmd;

bool AreCommandNamesValidAndEqual(const std::string& first,
                                  const std::string& second)
{
  return first.size() > 0 && second.size() > 0
    ? stricmp(first.data(), second.data()) == 0
    : false;
}
}

JsValue ConsoleApi::PrintConsole(const JsFunctionArguments& args)
{
  auto console = RE::ConsoleLog::GetSingleton();
  if (!console)
    throw NullPointerException("console");

  std::string s;

  for (size_t i = 1; i < args.GetSize(); ++i) {
    JsValue str = args[i];
    if (args[i].GetType() == JsValue::Type::Object &&
        !args[i].GetExternalData()) {

      JsValue json = JsValue::GlobalObject().GetProperty("JSON");
      str = json.GetProperty("stringify").Call({ json, args[i] });
    }
    s += str.ToString() + ' ';
  }

  int maxSize = 128;
  if (s.size() > maxSize) {
    s.resize(maxSize);
    s += "...";
  }
  console->Print("[Script] %s", s.data());

  return JsValue::Undefined();
}

void ConsoleApi::Clear()
{
  for (auto& item : replacedConsoleCmd) {
    SafeWriteBuf((uintptr_t)item.second.myIter, &(item.second.myOriginalData),
                 sizeof(item.second.myOriginalData));
  }

  replacedConsoleCmd.clear();
}

namespace {
ConsoleComand FillCmdInfo(ObScriptCommand* cmd)
{
  ConsoleComand cmdInfo;

  cmdInfo.longName = cmd->longName;
  cmdInfo.shortName = cmd->shortName;
  cmdInfo.numArgs = cmd->numParams;
  cmdInfo.execute = cmd->execute;
  cmdInfo.myIter = cmd;
  cmdInfo.myOriginalData = *cmd;
  cmdInfo.jsExecute = JsValue::Function(
    [](const JsFunctionArguments& args) { return JsValue::Bool(true); });

  return cmdInfo;
}

void CreateLongNameProperty(JsValue& obj, ConsoleComand* replaced)
{
  obj.SetProperty(
    "longName",
    [=](const JsFunctionArguments& args) {
      return JsValue::String(replaced->myIter->longName);
    },
    [=](const JsFunctionArguments& args) {
      replaced->longName = args[1].ToString();

      ObScriptCommand cmd = *replaced->myIter;
      cmd.longName = replaced->longName.c_str();

      SafeWriteBuf((uintptr_t)replaced->myIter, &cmd, sizeof(cmd));
      return JsValue::Undefined();
    });
}

void CreateShortNameProperty(JsValue& obj, ConsoleComand* replaced)
{
  obj.SetProperty(
    "shortName",
    [=](const JsFunctionArguments& args) {
      return JsValue::String(replaced->myIter->shortName);
    },
    [=](const JsFunctionArguments& args) {
      replaced->shortName = args[1].ToString();

      ObScriptCommand cmd = *replaced->myIter;
      cmd.shortName = replaced->shortName.c_str();

      SafeWriteBuf((uintptr_t)replaced->myIter, &cmd, sizeof(cmd));
      return JsValue::Undefined();
    });
}

void CreateNumArgsProperty(JsValue& obj, ConsoleComand* replaced)
{
  obj.SetProperty(
    "numArgs",
    [=](const JsFunctionArguments& args) {
      return JsValue::Double(replaced->myIter->numParams);
    },
    [=](const JsFunctionArguments& args) {
      replaced->numArgs = (double)args[1];

      ObScriptCommand cmd = *replaced->myIter;
      cmd.numParams = replaced->numArgs;

      SafeWriteBuf((uintptr_t)replaced->myIter, &cmd, sizeof(cmd));
      return JsValue::Undefined();
    });
}

void CreateExecuteProperty(JsValue& obj, ConsoleComand* replaced)
{
  obj.SetProperty("execute", nullptr, [=](const JsFunctionArguments& args) {
    replaced->jsExecute = args[1];
    return JsValue::Undefined();
  });
}
}

namespace {
struct ParseCommandResult
{
  std::string commandName;
  std::vector<std::string> params;
};

ParseCommandResult ParseCommand(std::string comand)
{
  ParseCommandResult res;
  static const std::string delimiterComa = ".";
  static const std::string delimiterSpase = " ";
  std::string token;

  size_t pos = comand.find(delimiterComa);
  if (pos != std::string::npos) {
    comand.erase(0, pos + delimiterComa.length());
  }

  while ((pos = comand.find(delimiterSpase)) != std::string::npos) {

    token = comand.substr(0, pos);
    res.commandName.empty() ? res.commandName = token
                            : res.params.push_back(token);
    comand.erase(0, pos + delimiterSpase.length());
  }

  if (comand.size() >= 1)
    res.params.push_back(comand);

  return res;
}

JsValue GetObject(const std::string& param)
{
  if (auto formByEditorId = RE::TESForm::LookupByEditorID(param))
    return JsValue::Double(formByEditorId->formID);

  auto id = strtoul(param.c_str(), nullptr, 16);

  if (auto formById = RE::TESForm::LookupByID(id))
    return JsValue::Double(formById->formID);

  auto err = "For param: " + param + " formId and editorId was not found";
  throw std::runtime_error(err.data());
}

JsValue GetTypedArg(RE::SCRIPT_PARAM_TYPE type, std::string param)
{
  switch (type) {
    case RE::SCRIPT_PARAM_TYPE::kStage:
    case RE::SCRIPT_PARAM_TYPE::kInt:
      return JsValue::Double((double)strtoll(param.c_str(), nullptr, 10));

    case RE::SCRIPT_PARAM_TYPE::kFloat:
      return JsValue::Double((double)strtod(param.c_str(), nullptr));

    case RE::SCRIPT_PARAM_TYPE::kCoontainerRef:
    case RE::SCRIPT_PARAM_TYPE::kInvObjectOrFormList:
    case RE::SCRIPT_PARAM_TYPE::kSpellItem:
    case RE::SCRIPT_PARAM_TYPE::kInventoryObject:
    case RE::SCRIPT_PARAM_TYPE::kPerk:
    case RE::SCRIPT_PARAM_TYPE::kActorBase:
    case RE::SCRIPT_PARAM_TYPE::kObjectRef:
      return JsValue::Double((double)strtoul(param.c_str(), nullptr, 16));

    case RE::SCRIPT_PARAM_TYPE::kAxis:
    case RE::SCRIPT_PARAM_TYPE::kActorValue:
    case RE::SCRIPT_PARAM_TYPE::kChar:
      return JsValue::String(param);

    default:
      return GetObject(param);
  }
}

bool ConsoleComand_Execute(const ObScriptParam* paramInfo,
                           ScriptData* scriptData, TESObjectREFR* thisObj,
                           TESObjectREFR* containingObj, Script* scriptObj,
                           ScriptLocals* locals, double& result,
                           UInt32& opcodeOffsetPtr)
{
  std::pair<const std::string, ConsoleComand>* iterator = nullptr;

  auto func = [&](int) {
    try {
      if (!scriptObj)
        throw NullPointerException("scriptObj");

      RE::Script* script = reinterpret_cast<RE::Script*>(scriptObj);

      std::string comand = script->GetCommand();
      auto parseCommandResult = ParseCommand(comand);

      for (auto& item : replacedConsoleCmd) {
        if (AreCommandNamesValidAndEqual(item.second.longName,
                                         parseCommandResult.commandName) ||
            AreCommandNamesValidAndEqual(item.second.shortName,
                                         parseCommandResult.commandName)) {

          std::vector<JsValue> args;
          args.push_back(JsValue::Undefined());
          auto refr = reinterpret_cast<RE::TESObjectREFR*>(thisObj);

          refr ? args.push_back(JsValue::Double((double)refr->formID))
               : args.push_back(JsValue::Double(0));
            

          auto param =
            reinterpret_cast<const RE::SCRIPT_PARAMETER*>(paramInfo);

          for (size_t i = 0; i < parseCommandResult.params.size(); ++i) {
            if (!param)
              break;

            JsValue arg =
              GetTypedArg(param[i].paramType, parseCommandResult.params[i]);

            if (arg.GetType() == JsValue::Type::Undefined) {
              auto err = " typeId " +
                std::to_string((uint32_t)param[i].paramType) +
                " not yet supported";

              throw std::runtime_error(err.data());
            }
            args.push_back(arg);
          }

          if (item.second.jsExecute.Call(args))
            iterator = &item;
          break;
        }
      }
    } catch (std::exception& e) {
      std::string what = e.what();
      g_taskQueue.AddTask([what] {
        throw std::runtime_error(what + " (in ConsoleComand_Execute)");
      });
    }
  };

  g_pool.push(func).wait();
  if (iterator)
    iterator->second.execute(paramInfo, scriptData, thisObj, containingObj,
                             scriptObj, locals, result, opcodeOffsetPtr);
  return true;
}

JsValue FindComand(ObScriptCommand* iter, size_t size,
                   const std::string& comandName)
{
  for (size_t i = 0; i < size; ++i) {
    ObScriptCommand* _iter = &iter[i];

    if (AreCommandNamesValidAndEqual(_iter->longName, comandName) ||
        AreCommandNamesValidAndEqual(_iter->shortName, comandName)) {
      JsValue obj = JsValue::Object();

      auto& replaced = replacedConsoleCmd[comandName];
      replaced = FillCmdInfo(_iter);

      CreateLongNameProperty(obj, &replaced);
      CreateShortNameProperty(obj, &replaced);
      CreateNumArgsProperty(obj, &replaced);
      CreateExecuteProperty(obj, &replaced);

      ObScriptCommand cmd = *_iter;
      cmd.execute = ConsoleComand_Execute;
      SafeWriteBuf((uintptr_t)_iter, &cmd, sizeof(cmd));
      return obj;
    }
  }
  return JsValue::Null();
}
}

JsValue ConsoleApi::FindConsoleComand(const JsFunctionArguments& args)
{
  auto comandName = args[1].ToString();

  JsValue res = FindComand(g_firstConsoleCommand, kObScript_NumConsoleCommands,
                           comandName);

  if (res.GetType() == JsValue::Type::Null)
    res = FindComand(g_firstObScriptCommand, kObScript_NumObScriptCommands,
                     comandName);

  return res;
}