#include "VirtualMachine.h"
#include "Utils.h"
#include <algorithm>
#include <stdexcept>

VirtualMachine::VirtualMachine(std::vector<PexScript::Lazy> loadedScripts)
{
  for (auto& script : loadedScripts) {
    allLoadedScripts[CIString{ script.source.begin(), script.source.end() }] =
      script;
  }
}

VirtualMachine::VirtualMachine(std::vector<PexScript::Ptr> loadedScripts)
{
  for (auto& script : loadedScripts) {
    allLoadedScripts[CIString{ script->source.begin(),
                               script->source.end() }] = {
      script->source, [script] { return script; }
    };
  }
}

std::string ToLower(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(), tolower);
  return s;
}

void VirtualMachine::RegisterFunction(std::string className,
                                      std::string functionName,
                                      FunctionType type, NativeFunction fn)
{
  switch (type) {
    case FunctionType::GlobalFunction:

      nativeStaticFunctions[ToLower(className)][ToLower(functionName)] = fn;
      break;
    case FunctionType::Method:
      nativeFunctions[ToLower(className)][ToLower(functionName)] = fn;
      break;
  }
}

void VirtualMachine::AddObject(IGameObject::Ptr self,
                               const std::vector<ScriptInfo>& scripts)
{
  std::vector<ActivePexInstance> scriptsForObject;

  for (auto& s : scripts) {
    CIString ciNameNeedScript{ s.name.begin(), s.name.end() };
    auto it = allLoadedScripts.find(ciNameNeedScript);
    if (it != allLoadedScripts.end()) {
      ActivePexInstance scriptInstance(it->second, s.vars, this,
                                       VarValue((IGameObject*)self.get()), "");
      scriptsForObject.push_back(scriptInstance);
    }
  }

  gameObjects[self] = scriptsForObject;
}

void VirtualMachine::SendEvent(IGameObject::Ptr self, const char* eventName,
                               const std::vector<VarValue>& arguments)
{
  for (auto& object : gameObjects) {
    if (object.first == self) {
      for (auto& scriptInstance : object.second) {
        auto name = scriptInstance.GetActiveStateName();

        auto fn = scriptInstance.GetFunctionByName(
          eventName, scriptInstance.GetActiveStateName());
        if (fn.valid) {
          scriptInstance.StartFunction(
            fn, const_cast<std::vector<VarValue>&>(arguments));
        }
      }
    }
  }
}

void VirtualMachine::SendEvent(ActivePexInstance* instance,
                               const char* eventName,
                               const std::vector<VarValue>& arguments)
{

  auto fn =
    instance->GetFunctionByName(eventName, instance->GetActiveStateName());
  if (fn.valid) {
    instance->StartFunction(fn, const_cast<std::vector<VarValue>&>(arguments));
  }
}

VarValue VirtualMachine::CallMethod(const std::string& activeInstanceName,
                                    VarValue* self, const char* methodName,
                                    std::vector<VarValue>& arguments)
{
  NativeFunction function;

  auto& activeSctipt = GetActivePexInObject(self, activeInstanceName);

  if (!activeSctipt.IsValid() || !self) {

    std::string error = "ActiveScript or self not valid!";

    throw std::runtime_error(error);
  }

  ActivePexInstance::Ptr currentParent = activeSctipt.GetParentInstance();
  std::string className = activeSctipt.GetSourcePexName();

  do {

    function = nativeFunctions[ToLower(className)][ToLower(methodName)];

    if (!function && currentParent->IsValid()) {

      className = currentParent->GetSourcePexName();
      currentParent = currentParent->GetParentInstance();
    }

  } while (!function && currentParent->IsValid());

  if (function)
    return function(*self, arguments);

  FunctionInfo functionInfo;

  std::string nameGoToState = "GotoState";
  std::string nameGetState = "GetState";

  if (methodName == nameGoToState || methodName == nameGetState) {
    functionInfo = activeSctipt.GetFunctionByName(methodName, "");
  } else
    functionInfo = activeSctipt.GetFunctionByName(
      methodName, activeSctipt.GetActiveStateName());

  if (functionInfo.valid) {
    return activeSctipt.StartFunction(functionInfo, arguments);
  }

  std::string name = "Method not found - '" + activeSctipt.GetSourcePexName() +
    "." + std::string(methodName) + "'";

  throw std::runtime_error(name);
}

VarValue VirtualMachine::CallStatic(std::string className,
                                    std::string functionName,
                                    std::vector<VarValue>& arguments)
{
  VarValue result = VarValue::None();
  FunctionInfo function;

  auto functionNameLower = ToLower(functionName);
  auto f = nativeStaticFunctions[ToLower(className)][functionNameLower]
    ? nativeStaticFunctions[ToLower(className)][functionNameLower]
    : nativeStaticFunctions[""][functionNameLower];

  if (f) {
    NativeFunction func = f;
    result = func(VarValue::None(), arguments);
    return result;
  }

  auto it =
    allLoadedScripts.find(CIString{ className.begin(), className.end() });
  if (it == allLoadedScripts.end())
    throw std::runtime_error("script not found - '" + className + "'");

  ActivePexInstance instance(it->second, nullptr, this, VarValue::None(), "");

  function = instance.GetFunctionByName(functionName.c_str(), "");

  if (function.valid) {
    if (function.IsNative())
      throw std::runtime_error("Function not found - '" +
                               std::string(functionName) + "'");

    result = instance.StartFunction(function, arguments);
  }
  if (!function.valid)
    throw std::runtime_error("function is not valid");

  return result;
}

ActivePexInstance& VirtualMachine::GetActivePexInObject(
  VarValue* object, const std::string& scriptType)
{
  static ActivePexInstance notValidInstance = ActivePexInstance();

  auto it = std::find_if(gameObjects.begin(), gameObjects.end(),
                         [&](RegisteredGameOgject& _object) {
                           for (auto& instance : _object.second) {
                             if (instance.GetSourcePexName() == scriptType) {
                               return true;
                             }
                           }
                           return false;
                         });

  if (it == gameObjects.end()) {
    return notValidInstance;
  }

  for (auto& instance : it->second) {
    if (instance.GetSourcePexName() == scriptType) {
      return instance;
    }
  }

  return notValidInstance;
}

PexScript::Lazy VirtualMachine::GetPexByName(const std::string& name)
{
  auto it = allLoadedScripts.find(CIString{ name.begin(), name.end() });
  if (it != allLoadedScripts.end())
    return it->second;
  return PexScript::Lazy();
}

ActivePexInstance::Ptr VirtualMachine::CreateActivePexInstance(
  const std::string& pexScriptName, VarValue activeInstanceOwner,
  const std::shared_ptr<IVariablesHolder>& mapForFillPropertys,
  std::string childrenName)
{

  auto it = allLoadedScripts.find(
    CIString{ pexScriptName.begin(), pexScriptName.end() });
  if (it != allLoadedScripts.end()) {
    ActivePexInstance scriptInstance(it->second, mapForFillPropertys, this,
                                     activeInstanceOwner, childrenName);
    return std::make_shared<ActivePexInstance>(scriptInstance);
  }

  static const ActivePexInstance::Ptr notValidInstance =
    std::make_shared<ActivePexInstance>();

  if (pexScriptName != "")
    throw std::runtime_error("Unable to find script '" + pexScriptName + "'");

  return notValidInstance;
}

bool VirtualMachine::IsNativeFunctionByNameExisted(
  const std::string& name) const
{
  for (auto& staticFunction : nativeStaticFunctions) {
    if (staticFunction.first == name)
      return true;
  }

  for (auto& metod : nativeFunctions) {
    for (auto& func : metod.second)
      if (func.first == name)
        return true;
  }

  return false;
}

void VirtualMachine::RemoveObject(IGameObject::Ptr self)
{
}