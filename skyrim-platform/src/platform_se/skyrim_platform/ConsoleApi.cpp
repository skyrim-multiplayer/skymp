#include "ConsoleApi.h"
#include "InGameConsolePrinter.h"
#include "InvalidArgumentException.h"
#include "NullPointerException.h"
#include "SkyrimPlatform.h"
#include "ThreadPoolWrapper.h"
#include "Validators.h"
#include "WindowsConsolePrinter.h"

std::shared_ptr<IConsolePrinter> g_printer(new InGameConsolePrinter);
std::shared_ptr<IConsolePrinter> g_windowsConsolePrinter = nullptr;

namespace {
struct ConsoleCommand
{
  std::string longName;
  std::string shortName;
  uint16_t numArgs = 0;
  RE::SCRIPT_FUNCTION::Execute_t* execute;
  std::shared_ptr<Napi::Reference<Napi::Function>> jsExecute;
  RE::SCRIPT_FUNCTION* myIter;
  RE::SCRIPT_FUNCTION myOriginalData;
};
static std::map<std::string, ConsoleCommand> g_replacedConsoleCmd;
static bool g_printConsolePrefixesEnabled = true;
static std::shared_ptr<Napi::Env> g_envForExecuteCallback; // TODO

bool IsNameEqual(const std::string& first, const std::string& second)
{
  return first.size() > 0 && second.size() > 0
    ? stricmp(first.data(), second.data()) == 0
    : false;
}
} // namespace

Napi::Value ConsoleApi::PrintConsole(const Napi::CallbackInfo &info)
{
  g_printer->Print(info);

  if (g_windowsConsolePrinter) {
    g_windowsConsolePrinter->Print(info);
  }

  return info.Env().Undefined();
}

void ConsoleApi::Clear()
{
  for (auto& item : g_replacedConsoleCmd) {
    REL::safe_write((uintptr_t)item.second.myIter,
                    &(item.second.myOriginalData),
                    sizeof(item.second.myOriginalData));
  }

  g_replacedConsoleCmd.clear();
}

const char* ConsoleApi::GetScriptPrefix()
{
  return g_printConsolePrefixesEnabled ? "[Script] " : "";
}

const char* ConsoleApi::GetExceptionPrefix()
{
  return g_printConsolePrefixesEnabled ? "[Exception] " : "";
}

void ConsoleApi::InitCmd(int offsetLeft, int offsetTop, int width, int height,
                         bool isAlwaysOnTop)
{

  g_windowsConsolePrinter = std::make_shared<WindowsConsolePrinter>(
    offsetLeft, offsetTop, width, height, isAlwaysOnTop);
}

namespace {
ConsoleCommand FillCmdInfo(Napi::Env env, RE::SCRIPT_FUNCTION* cmd)
{
  ConsoleCommand cmdInfo;

  cmdInfo.longName = cmd->functionName;
  cmdInfo.shortName = cmd->shortName;
  cmdInfo.numArgs = cmd->numParams;
  cmdInfo.execute = cmd->executeFunction;
  cmdInfo.myIter = cmd;
  cmdInfo.myOriginalData = *cmd;
  cmdInfo.jsExecute = Napi::Function::New(env,
    [](const Napi::CallbackInfo &info) { return Napi::Boolean::New(info.Env(), true); });

  return cmdInfo;
}

void CreateLongNameProperty(Napi::Object& obj, ConsoleCommand* replaced) {
  auto getter = [=](const Napi::CallbackInfo &info) -> Napi::Value {
    return Napi::String::New(info.Env(), replaced->myIter->functionName);
  };

  auto setter = [=](const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
      Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
      return;
    }

    replaced->longName = info[0].As<Napi::String>().Utf8Value();

    RE::SCRIPT_FUNCTION cmd = *replaced->myIter;
    cmd.functionName = replaced->longName.c_str();

    REL::safe_write((uintptr_t)replaced->myIter, &cmd, sizeof(cmd));
  };

  Napi::PropertyDescriptor longNameProperty = Napi::PropertyDescriptor::Accessor(
    "longName", getter, setter, napi_default
  );

  obj.DefineProperty(longNameProperty);
}

void CreateShortNameProperty(Napi::Object& obj, ConsoleCommand* replaced) {
  auto getter = [=](const Napi::CallbackInfo &info) -> Napi::Value {
    return Napi::String::New(info.Env(), replaced->myIter->shortName);
  };

  auto setter = [=](const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
      Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
      return;
    }

    replaced->shortName = info[0].As<Napi::String>().Utf8Value();

    RE::SCRIPT_FUNCTION cmd = *replaced->myIter;
    cmd.shortName = replaced->shortName.c_str();

    REL::safe_write((uintptr_t)replaced->myIter, &cmd, sizeof(cmd));
  };

  Napi::PropertyDescriptor shortNameProperty = Napi::PropertyDescriptor::Accessor(
    "shortName", getter, setter, napi_default
  );

  obj.DefineProperty(shortNameProperty);
}

void CreateNumArgsProperty(Napi::Object& obj, ConsoleCommand* replaced) {
  auto getter = [=](const Napi::CallbackInfo &info) -> Napi::Value {
    return Napi::Number::New(info.Env(), replaced->myIter->numParams);
  };

  auto setter = [=](const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
      Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
      return;
    }

    replaced->numArgs = info[0].As<Napi::Number>().DoubleValue();

    RE::SCRIPT_FUNCTION cmd = *replaced->myIter;
    cmd.numParams = replaced->numArgs;

    REL::safe_write((uintptr_t)replaced->myIter, &cmd, sizeof(cmd));
  };

  Napi::PropertyDescriptor numArgsProperty = Napi::PropertyDescriptor::Accessor(
    "numArgs", getter, setter, napi_default
  );

  obj.DefineProperty(numArgsProperty);
}

void CreateExecuteProperty(Napi::Value& obj, ConsoleCommand* replaced)
{
  obj.Set("execute", nullptr, [=](const Napi::CallbackInfo &info) {
    replaced->jsExecute = info[0];
    return info.Env().Undefined();
  });
}

struct ParseCommandResult
{
  std::string commandName;
  std::vector<std::string> params;
};

ParseCommandResult ParseCommand(std::string command)
{
  ParseCommandResult res;
  static const std::string delimiterComa = ".";
  static const std::string delimiterSpase = " ";
  std::string token;

  size_t pos = command.find(delimiterComa);
  if (pos != std::string::npos) {
    command.erase(0, pos + delimiterComa.length());
  }

  while ((pos = command.find(delimiterSpase)) != std::string::npos) {

    token = command.substr(0, pos);
    if (res.commandName.empty()) {
      res.commandName = token;
    } else {
      res.params.push_back(token);
    }
    command.erase(0, pos + delimiterSpase.length());
  }

  if (command.size() >= 1)
    res.params.push_back(command);

  return res;
}

Napi::Value GetObject(Napi::Env env, const std::string& param)
{
  if (auto formByEditorId = RE::TESForm::LookupByEditorID(param))
    return Napi::Number::New(env, formByEditorId->formID);

  auto id = strtoul(param.c_str(), nullptr, 16);

  if (auto formById = RE::TESForm::LookupByID(id))
    return Napi::Number::New(env, formById->formID);

  auto err = "For param: " + param + " formId and editorId was not found";
  throw std::runtime_error(err.data());
}

Napi::Value GetTypedArg(Napi::Env env, RE::SCRIPT_PARAM_TYPE type, std::string param)
{
  switch (type) {
    case RE::SCRIPT_PARAM_TYPE::kStage:
    case RE::SCRIPT_PARAM_TYPE::kInt:
      return Napi::Number::New(env, (double)strtoll(param.c_str(), nullptr, 10));

    case RE::SCRIPT_PARAM_TYPE::kFloat:
      return Napi::Number::New(env, (double)strtod(param.c_str(), nullptr));

      // RE::SCRIPT_PARAM_TYPE::kContainerRef/kCoontainerRef
    case static_cast<RE::SCRIPT_PARAM_TYPE>(0x1A):

    case RE::SCRIPT_PARAM_TYPE::kInvObjectOrFormList:
    case RE::SCRIPT_PARAM_TYPE::kSpellItem:
    case RE::SCRIPT_PARAM_TYPE::kInventoryObject:
    case RE::SCRIPT_PARAM_TYPE::kPerk:
    case RE::SCRIPT_PARAM_TYPE::kActorBase:
    case RE::SCRIPT_PARAM_TYPE::kObjectRef:
      return Napi::Number::New(env, (double)strtoul(param.c_str(), nullptr, 16));

    case RE::SCRIPT_PARAM_TYPE::kAxis:
    case RE::SCRIPT_PARAM_TYPE::kActorValue:
    case RE::SCRIPT_PARAM_TYPE::kChar:
      return Napi::Value::String(param);

    default:
      return GetObject(env, param);
  }
}

bool ConsoleComand_Execute(const RE::SCRIPT_PARAMETER* paramInfo,
                           RE::SCRIPT_FUNCTION::ScriptData* scriptData,
                           RE::TESObjectREFR* thisObj,
                           RE::TESObjectREFR* containingObj,
                           RE::Script* scriptObj, RE::ScriptLocals* locals,
                           double& result, std::uint32_t& opcodeOffsetPtr)
{
  std::pair<const std::string, ConsoleCommand>* iterator = nullptr;

  auto func = [&] {
    try {
      if (!g_envForExecuteCallback)
        throw NullPointerException("g_envForExecuteCallback");
      auto env = *g_envForExecuteCallback;

      if (!scriptObj)
        throw NullPointerException("scriptObj");

      std::string command = scriptObj->GetCommand();
      auto parseCommandResult = ParseCommand(command);

      for (auto& item : g_replacedConsoleCmd) {
        if (IsNameEqual(item.second.longName,
                        parseCommandResult.commandName) ||
            IsNameEqual(item.second.shortName,
                        parseCommandResult.commandName)) {

          std::vector<Napi::Value> args;
          auto refr = reinterpret_cast<RE::TESObjectREFR*>(thisObj);

          refr ? args.push_back(Napi::Number::New(env, (double)refr->formID))
               : args.push_back(Napi::Number::New(env, 0));

          for (size_t i = 0; i < parseCommandResult.params.size(); ++i) {
            if (!paramInfo)
              break;

            Napi::Value arg = GetTypedArg(env, paramInfo[i].paramType.get(),
                                      parseCommandResult.params[i]);

            if (arg.IsUndefined()) {
              auto err = " typeId " +
                std::to_string((uint32_t)paramInfo[i].paramType.get()) +
                " not yet supported";

              throw std::runtime_error(err.data());
            }
            args.push_back(arg);
          }

          if (item.second.jsExecute->Value().Call(env.Undefined(), args))
            iterator = &item;
          break;
        }
      }
    } catch (std::exception& e) {
      std::string what = e.what();
      SkyrimPlatform::GetSingleton()->AddUpdateTask([what] {
        throw std::runtime_error(what + " (in ConsoleCommand_Execute)");
      });
    }
  };

  SkyrimPlatform::GetSingleton()->PushAndWait(func);
  if (iterator)
    iterator->second.execute(paramInfo, scriptData, thisObj, containingObj,
                             scriptObj, locals, result, opcodeOffsetPtr);
  return true;
}

Napi::Value FindCommand(Napi::Env env, const std::string& commandName, RE::SCRIPT_FUNCTION* start,
                    size_t count)
{
  for (size_t i = 0; i < count; ++i) {
    RE::SCRIPT_FUNCTION* _iter = &start[i];

    if (IsNameEqual(_iter->functionName, commandName) ||
        IsNameEqual(_iter->shortName, commandName)) {
      Napi::Value obj = Napi::Object::New(env);

      auto& replaced = g_replacedConsoleCmd[commandName];
      replaced = FillCmdInfo(env, _iter);

      CreateLongNameProperty(obj, &replaced);
      CreateShortNameProperty(obj, &replaced);
      CreateNumArgsProperty(obj, &replaced);
      CreateExecuteProperty(obj, &replaced);

      RE::SCRIPT_FUNCTION cmd = *_iter;
      cmd.executeFunction = ConsoleComand_Execute;
      REL::safe_write((uintptr_t)_iter, &cmd, sizeof(cmd));
      return obj;
    }
  }
  return env.Null();
}
} // namespace

Napi::Value ConsoleApi::FindConsoleCommand(const Napi::CallbackInfo &info)
{
  auto commandName = NapiHelper::ExtractString(info[0], "commandName");

  Napi::Value res =
    FindCommand(env, commandName, RE::SCRIPT_FUNCTION::GetFirstConsoleCommand(),
                RE::SCRIPT_FUNCTION::Commands::kConsoleCommandsEnd);

  if (res.IsNull()) {
    res =
      FindCommand(env, commandName, RE::SCRIPT_FUNCTION::GetFirstScriptCommand(),
                  RE::SCRIPT_FUNCTION::Commands::kScriptCommandsEnd);
  }

  return res;
}

Napi::Value ConsoleApi::WriteLogs(const Napi::CallbackInfo &info)
{
  auto pluginName = NapiHelper::ExtractString(info[0], "pluginName");
  if (!ValidateFilename(pluginName, /*allowDots*/ false)) {
    throw InvalidArgumentException("pluginName", pluginName);
  }

  static std::map<std::string, std::unique_ptr<std::ofstream>> g_m;

  if (!g_m[pluginName]) {
    g_m[pluginName] = std::make_unique<std::ofstream>(
      "Data\\Platform\\Logs\\" + pluginName + "-logs.txt");
  }

  std::string s;

  for (size_t i = 1; i < info.Length(); ++i) {
    Napi::Value str = info[i];

    if (info[i].IsObject() && !info[i].IsExternal()) {
      Napi::Object global = env.Global();
      Napi::Object json = global.Get("JSON").As<Napi::Object>();

      Napi::Function stringify = json.Get("stringify").As<Napi::Function>();
      str = stringify.Call(json, { info[i] });
    }

    s += str.As<Napi::String>().Utf8Value() + ' ';
  }

  (*g_m[pluginName]) << s << std::endl;
  return info.Env().Undefined();
}

Napi::Value ConsoleApi::SetPrintConsolePrefixesEnabled(
  const Napi::CallbackInfo &info)
{
  g_printConsolePrefixesEnabled = static_cast<bool>(args[1]);
  return info.Env().Undefined();
}
