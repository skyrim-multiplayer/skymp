#include "papyrus-vm/VirtualMachine.h"
#include "papyrus-vm/Utils.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace {
constexpr uint32_t g_maxStackId = 100'000;
}

VirtualMachine::VirtualMachine(
  const std::vector<PexScript::Lazy>& loadedScripts)
{
  stackIdMaker.reset(new MakeID(g_maxStackId));

  for (auto& script : loadedScripts) {
    allLoadedScripts[CIString{ script.source.begin(), script.source.end() }] =
      script;
  }
}

VirtualMachine::VirtualMachine(
  const std::vector<std::shared_ptr<PexScript>>& loadedScripts)
{
  stackIdMaker.reset(new MakeID(g_maxStackId));

  for (auto& script : loadedScripts) {
    allLoadedScripts[CIString{ script->source.begin(),
                               script->source.end() }] = {
      script->source, [script] { return script; }
    };
  }
}

void VirtualMachine::SetMissingScriptHandler(
  const MissingScriptHandler& handler)
{
  this->missingScriptHandler = handler;
}

void VirtualMachine::SetExceptionHandler(const ExceptionHandler& handler)
{
  this->handler = handler;
}

std::string ToLower(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(), tolower);
  return s;
}

void VirtualMachine::RegisterFunction(const std::string& className,
                                      const std::string& functionName,
                                      const FunctionType& type,
                                      const NativeFunction& fn)
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

void VirtualMachine::AddObject(std::shared_ptr<IGameObject> self,
                               const std::vector<ScriptInfo>& scripts)
{
  std::vector<std::shared_ptr<ActivePexInstance>> scriptsForObject;

  for (auto& s : scripts) {
    CIString ciNameNeedScript{ s.name.begin(), s.name.end() };
    auto it = allLoadedScripts.find(ciNameNeedScript);
    if (it != allLoadedScripts.end()) {
      auto scriptInstance = std::make_shared<ActivePexInstance>(
        it->second, s.vars, this, VarValue((IGameObject*)self.get()), "");
      scriptsForObject.push_back(scriptInstance);
    }
  }

  self->activePexInstances = scriptsForObject;
  gameObjectsHolder.insert(self);
}

void VirtualMachine::SendEvent(std::shared_ptr<IGameObject> self,
                               const char* eventName,
                               const std::vector<VarValue>& arguments,
                               OnEnter enter)
{
  for (auto& scriptInstance : self->activePexInstances) {
    auto name = scriptInstance->GetActiveStateName();

    auto fn = scriptInstance->GetFunctionByName(
      eventName, scriptInstance->GetActiveStateName());
    if (fn.valid) {
      auto stackIdHolder = std::make_shared<StackIdHolder>(*this);
      if (enter)
        enter(*stackIdHolder);
      scriptInstance->StartFunction(
        fn, const_cast<std::vector<VarValue>&>(arguments), stackIdHolder);
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
    instance->StartFunction(fn, const_cast<std::vector<VarValue>&>(arguments),
                            std::make_shared<StackIdHolder>(*this));
  }
}

StackIdHolder::StackIdHolder(VirtualMachine& vm_)
  : makeId(vm_.stackIdMaker)
{
  if (!makeId->CreateID(*reinterpret_cast<uint32_t*>(&stackId))) {
    throw std::runtime_error("CreateID failed to create id for stack");
  }
}

StackIdHolder::~StackIdHolder()
{
  bool destroyed = makeId->DestroyID(stackId);
  (void)destroyed;
  assert(destroyed);
}

int32_t StackIdHolder::GetStackId() const
{
  return stackId;
}

VarValue VirtualMachine::CallMethod(
  IGameObject* selfObj, const char* methodName,
  std::vector<VarValue>& arguments,
  std::shared_ptr<StackIdHolder> stackIdHolder)
{
  if (!stackIdHolder) {
    stackIdHolder.reset(new StackIdHolder(*this));
  }

  if (!selfObj) {
    return VarValue::None();
  }

  const char* nativeClass = selfObj->GetParentNativeScript();
  const char* base = nativeClass;
  while (1) {
    if (auto f = nativeFunctions[ToLower(base)][ToLower(methodName)]) {
      auto self = VarValue(selfObj);
      self.SetMetaStackIdHolder(stackIdHolder);
      return f(self, arguments);
    }
    auto it = allLoadedScripts.find(base);
    if (it == allLoadedScripts.end())
      break;
    base = it->second.fn()->objectTable[0].parentClassName.data();
    if (!base[0])
      break;
  }

  for (auto& activeScript : selfObj->activePexInstances) {
    FunctionInfo functionInfo;

    if (!Utils::stricmp(methodName, "GotoState") ||
        !Utils::stricmp(methodName, "GetState")) {
      functionInfo = activeScript->GetFunctionByName(methodName, "");
    } else {
      functionInfo = activeScript->GetFunctionByName(
        methodName, activeScript->GetActiveStateName());
      if (!functionInfo.valid)
        functionInfo = activeScript->GetFunctionByName(methodName, "");
    }

    if (functionInfo.valid) {
      return activeScript->StartFunction(functionInfo, arguments,
                                         stackIdHolder);
    }
  }

  std::string e = "Method not found - '";
  e += base;
  e += (base[0] ? "." : "") + std::string(methodName) + "'";
  throw std::runtime_error(e);
}

VarValue VirtualMachine::CallStatic(
  const std::string& className, const std::string& functionName,
  std::vector<VarValue>& arguments,
  std::shared_ptr<StackIdHolder> stackIdHolder)
{
  if (!stackIdHolder) {
    stackIdHolder.reset(new StackIdHolder(*this));
  }

  VarValue result = VarValue::None();
  FunctionInfo function;

  auto functionNameLower = ToLower(functionName);
  auto f = nativeStaticFunctions[ToLower(className)][functionNameLower]
    ? nativeStaticFunctions[ToLower(className)][functionNameLower]
    : nativeStaticFunctions[""][functionNameLower];

  if (f) {
    auto self = VarValue::None();
    self.SetMetaStackIdHolder(stackIdHolder);
    return f(self, arguments);
  }

  auto classNameCi = CIString{ className.begin(), className.end() };
  auto it = allLoadedScripts.find(classNameCi);
  if (it == allLoadedScripts.end()) {
    if (this->missingScriptHandler) {
      if (auto newScript = this->missingScriptHandler(className)) {
        allLoadedScripts[classNameCi] = *newScript;
      }
    }
    it = allLoadedScripts.find(classNameCi);
    if (it == allLoadedScripts.end())
      throw std::runtime_error("script is missing - '" + className + "'");
  }

  auto& instance = instancesForStaticCalls[className];
  if (!instance) {
    instance = std::make_shared<ActivePexInstance>(it->second, nullptr, this,
                                                   VarValue::None(), "");
  }

  function = instance->GetFunctionByName(functionName.c_str(), "");

  if (function.valid) {
    if (function.IsNative()) {
      throw std::runtime_error("Function not found - '" +
                               std::string(functionName) + "'");
    }

    result = instance->StartFunction(function, arguments, stackIdHolder);
  }
  if (!function.valid) {
    throw std::runtime_error("Function is not valid - '" +
                             std::string(functionName) + "'");
  }

  return result;
}

PexScript::Lazy VirtualMachine::GetPexByName(const std::string& name)
{
  auto it = allLoadedScripts.find(CIString{ name.begin(), name.end() });
  if (it != allLoadedScripts.end())
    return it->second;
  return PexScript::Lazy();
}

std::shared_ptr<ActivePexInstance> VirtualMachine::CreateActivePexInstance(
  const std::string& pexScriptName, VarValue activeInstanceOwner,
  const std::shared_ptr<IVariablesHolder>& mapForFillProperties,
  const std::string& childrenName)
{

  auto it = allLoadedScripts.find(
    CIString{ pexScriptName.begin(), pexScriptName.end() });
  if (it != allLoadedScripts.end()) {
    ActivePexInstance scriptInstance(it->second, mapForFillProperties, this,
                                     activeInstanceOwner, childrenName);
    return std::make_shared<ActivePexInstance>(scriptInstance);
  }

  static const std::shared_ptr<ActivePexInstance> notValidInstance =
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

VirtualMachine::ExceptionHandler VirtualMachine::GetExceptionHandler() const
{
  return handler;
}

void VirtualMachine::RemoveObject(std::shared_ptr<IGameObject> self)
{
}
