#include "papyrus-vm/VirtualMachine.h"
#include "antigo/Context.h"
#include "antigo/ResolvedContext.h"
#include "papyrus-vm/Utils.h"
#include <algorithm>
#include <cassert>
#include <exception>
#include <fmt/format.h>
#include <limits>
#include <memory>
#include <random>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>

namespace {
constexpr uint32_t g_maxStackId = 100'000;
}


void StackData::EnableTracing(Antigo::OnstackContext& parentCtx) {
  thread_local std::random_device rd;
  thread_local std::mt19937 gen(rd());
  thread_local std::uniform_int_distribution<size_t> dist(std::numeric_limits<size_t>::max() / 2, std::numeric_limits<size_t>::max());
  if (tracing.enabled) {
    return;
  }
  tracing.enabled = true;
  // tracing.traceId = std::chrono::steady_clock::now().time_since_epoch().count();
  tracing.traceId = dist(gen);
  spdlog::info("TRACING PAPYRUS STACK {}-{}: enabled", stackIdHolder.GetStackId(), tracing.traceId);
}

std::vector<std::weak_ptr<StackData>> StackData::activeStacks{};

std::shared_ptr<StackData> StackData::Create(VirtualMachine& vm_) {
  auto p = std::make_shared<StackData>(vm_, InternalToken{});
  if (activeStacks.empty()) {
    activeStacks.resize(g_maxStackId);
    size_t id = p->stackIdHolder.GetStackId();
    if (id >= activeStacks.size()) {
      spdlog::error("bad stack id: {} >= {}", id, activeStacks.size());
      return p;
    }
    auto& ptrInArr = activeStacks[id];
    if (ptrInArr.use_count() != 0) {
      spdlog::error("duplicate stack id: {}, old use count", id, ptrInArr.use_count());
    }
    ptrInArr = p;
  }
  return p;
}

StackData::StackData(VirtualMachine& vm_, InternalToken): stackIdHolder(vm_) {}

StackData::~StackData() {
  ANTIGO_CONTEXT_INIT(ctx);
  if (tracing.enabled) {
    std::string last;
    if (tracing.msgs.empty()) {
      last = "(empty)";
    } else {
      last = tracing.msgs.back();
    }
    std::string resolved;
    try {
      resolved = ctx.Resolve().ToString();
    } catch (const std::exception& e) {
      resolved = std::string{"unexpected error during resolving: "} + e.what();
    }
    spdlog::info("TRACING PAPYRUS STACK {}-{}: DESTRUCTOR (last: {})\n{}", stackIdHolder.GetStackId(), tracing.traceId, last, resolved);
  }
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

      nativeStaticFunctions[className.data()][functionName.data()] = fn;
      break;
    case FunctionType::Method:
      nativeFunctions[className.data()][functionName.data()] = fn;
      break;
  }
}

std::set<CIString> VirtualMachine::ListClasses() const
{
  std::set<CIString> result;

  for (auto& f : nativeStaticFunctions) {
    result.insert(f.first.data());
  }

  for (auto& f : nativeFunctions) {
    result.insert(f.first.data());
  }

  return result;
}

CIString VirtualMachine::GetBaseClass(const CIString& className) const
{
  auto it = allLoadedScripts.find(className);
  if (it != allLoadedScripts.end()) {
    return it->second.fn()->objectTable[0].parentClassName.data();
  }
  return CIString();
}

std::set<CIString> VirtualMachine::ListStaticFunctions(
  const CIString& className) const
{
  std::set<CIString> result;

  auto it = nativeStaticFunctions.find(className.data());
  if (it != nativeStaticFunctions.end()) {
    for (auto& f : it->second) {
      result.insert(f.first.data());
    }
  }

  return result;
}

std::set<CIString> VirtualMachine::ListMethods(const CIString& className) const
{
  std::set<CIString> result;

  auto it = nativeFunctions.find(className.data());
  if (it != nativeFunctions.end()) {
    for (auto& f : it->second) {
      result.insert(f.first.data());
    }
  }

  return result;
}

NativeFunction VirtualMachine::GetFunctionImplementation(
  const CIString& className, const CIString& functionName, bool isStatic) const
{

  if (!isStatic) {
    auto it = nativeFunctions.find(className.data());
    if (it != nativeFunctions.end()) {
      auto it2 = it->second.find(functionName.data());
      if (it2 != it->second.end()) {
        return it2->second;
      }
    }
  }

  if (isStatic) {
    auto it3 = nativeStaticFunctions.find(className.data());
    if (it3 != nativeStaticFunctions.end()) {
      auto it4 = it3->second.find(functionName.data());
      if (it4 != it3->second.end()) {
        return it4->second;
      }
    }
  }

  return nullptr;
}

bool VirtualMachine::DynamicCast(const VarValue& object,
                                 const CIString& className) const
{
  VarValue scriptToCastOwner = object;
  VarValue result;
  result.objectType = className.data();
  ActivePexInstance::CastObjectToObject(*this, &result, &scriptToCastOwner);

  if (static_cast<IGameObject*>(result) != nullptr) {
    return true;
  }

  return false;
}

void VirtualMachine::AddObject(std::shared_ptr<IGameObject> self,
                               const std::vector<ScriptInfo>& scripts)
{
  ANTIGO_CONTEXT_INIT(ctx);

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

  for (auto& script : scriptsForObject) {
    self->AddScript(script);
  }
  gameObjectsHolder.insert(self);
  // XXX why
}

void VirtualMachine::SendEvent(std::shared_ptr<IGameObject> self,
                               const char* eventName,
                               const std::vector<VarValue>& arguments,
                               OnEnter enter)
{
  ANTIGO_CONTEXT_INIT(ctx);
  ctx.AddMessage("next: self, eventName");
  ctx.AddPtr(self);
  std::string eventNameS = eventName;
  ctx.AddLambdaWithOwned([n = std::move(eventNameS)]() { return n; });
  // XXX bad naming, outer scope
  ctx.AddLambdaWithOwned([&arguments, &enter]{
    std::stringstream ss;
    ss << "arguments = [" << arguments.size() << "] [\n";
    for (const auto& arg : arguments) {
      ss << "  " << arg.ToString() << "\n";
    }
    ss << "]\nenter as bool = " << static_cast<bool>(enter);
    return std::move(ss).str();
  });

  for (auto& scriptInstance : self->ListActivePexInstances()) {
    ctx.AddLambdaWithOwned([pexName = scriptInstance->GetSourcePexName()]{return "pex = " + pexName;});
    auto name = scriptInstance->GetActiveStateName();
    ctx.AddLambdaWithOwned([name]{return "state = " + name;});

    auto fn = scriptInstance->GetFunctionByName(
      eventName, scriptInstance->GetActiveStateName());
    if (fn.valid) {
      ctx.AddMessage("valid");
      auto stackData = StackData::Create(*this);
      if (strcmp(eventName, "OnHit") == 0) {
        stackData->EnableTracing(ctx);
      }
      if (enter) {
        enter(*stackData);
      }
      scriptInstance->StartFunction(
        fn, const_cast<std::vector<VarValue>&>(arguments), stackData);
    } else {
      ctx.AddMessage("invalid");
    }
  }
}

void VirtualMachine::SendEvent(ActivePexInstance* instance,
                               const char* eventName,
                               const std::vector<VarValue>& arguments)
{
  ANTIGO_CONTEXT_INIT(ctx);
  ctx.AddMessage("next: instance, eventName");
  ctx.AddPtr(instance);
  std::string eventNameS = eventName;
  ctx.AddLambdaWithOwned([n = std::move(eventNameS)]() { return n; });

  auto fn =
    instance->GetFunctionByName(eventName, instance->GetActiveStateName());
  if (fn.valid) {
    auto stackData = StackData::Create(*this);
    instance->StartFunction(fn, const_cast<std::vector<VarValue>&>(arguments),
                            stackData);
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

StackDepthHolder::StackDepthHolder() = default;

size_t StackDepthHolder::GetStackDepth() const
{
  return depth;
}

void StackDepthHolder::IncreaseStackDepth()
{
  ++depth;
}

void StackDepthHolder::DecreaseStackDepth()
{
  --depth;
}

VarValue VirtualMachine::CallMethod(
  IGameObject* selfObj, const char* methodName,
  std::vector<VarValue>& arguments, std::shared_ptr<StackData> stackData,
  const std::vector<std::shared_ptr<ActivePexInstance>>*
    activePexInstancesOverride)
{
  ANTIGO_CONTEXT_INIT(ctx);
  auto g = ctx.AddLambdaWithRef([selfObj, methodName, arguments]() {
    std::stringstream ss;
    ss << "selfObj = ";
    if (selfObj) {
      ss << selfObj->GetStringID();
    } else {
      ss << "nullptr";
    }
    ss << "\n";
    ss << "methodName = " << methodName << "\n";
    ss << "arguments = [\n";
    for (const auto& arg : arguments) {
      ss << "  " << arg.ToString() << "\n";
    }
    ss << "]";
    return std::move(ss).str();
  });
  g.Arm();

  if (!stackData) {
    stackData = StackData::Create(*this);
  }

  if (!selfObj) {
    ctx.AddMessage("tried to call a method for null");
    ctx.Resolve().Print();
    return VarValue::None();
  }

  const auto& instances = activePexInstancesOverride
    ? *activePexInstancesOverride
    : selfObj->ListActivePexInstances();

  // TODO: in theory we shouldn't iterate over all scripts, but only use the
  // current one
  for (auto& activeScript : instances) {
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
      return activeScript->StartFunction(functionInfo, arguments, stackData);
    }
  }

  // natives have to be after non-natives (Bethesda overrides native functions
  // in some scripts)
  const char* nativeClass = selfObj->GetParentNativeScript();
  const char* base = nativeClass;
  while (1) {
    if (auto f = nativeFunctions[base][methodName]) {
      auto self = VarValue(selfObj);
      self.SetMetaStackIdHolder(stackData->stackIdHolder);
      return f(self, arguments);
    }
    auto it = allLoadedScripts.find(base);
    if (it == allLoadedScripts.end())
      break;
    base = it->second.fn()->objectTable[0].parentClassName.data();
    if (!base[0])
      break;
  }

  // ctx.AddMessage("method not found (see adjacent message if this one came from an exception)");
  // ctx.Orphan();

  ANTIGO_CONTEXT_INIT(ctx_hack); // a way to avoid ref expiry

  std::string e = "Method not found - '";
  e += base;
  e += (base[0] ? "." : "") + std::string(methodName) + "'";
  throw std::runtime_error(e);
}

VarValue VirtualMachine::CallStatic(const std::string& className,
                                    const std::string& functionName,
                                    std::vector<VarValue>& arguments,
                                    std::shared_ptr<StackData> stackData)
{
  ANTIGO_CONTEXT_INIT(ctx);

  ctx.AddLambdaWithOwned([&className, &functionName, &arguments, &stackData]{
    std::stringstream ss;
    ss << "className = " << className << "\n";
    ss << "functionName = " << functionName << "\n";
    ss << "arguments [" << arguments.size() << "] = [\n";
    for (const auto& arg : arguments) {
      ss << "  " << arg.ToString() << "\n";
    }
    ss << "]\nstackData = " << std::hex << stackData;
    return std::move(ss).str();
  });

  if (!stackData) {
    stackData = StackData::Create(*this);
  }

  VarValue result = VarValue::None();
  FunctionInfo function;

  auto f = nativeStaticFunctions[className.data()][functionName.data()]
    ? nativeStaticFunctions[className.data()][functionName.data()]
    : nativeStaticFunctions[""][functionName.data()];

  if (f) {
    auto self = VarValue::None();
    self.SetMetaStackIdHolder(stackData->stackIdHolder);
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

    result = instance->StartFunction(function, arguments, stackData);
  }
  if (!function.valid) {
    throw std::runtime_error("Function is not valid - '" +
                             std::string(functionName) + "'");
  }

  return result;
}

PexScript::Lazy VirtualMachine::GetPexByName(const std::string& name) const
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
  ANTIGO_CONTEXT_INIT(ctx);

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
    if (staticFunction.first == name.data())
      return true;
  }

  for (auto& metod : nativeFunctions) {
    for (auto& func : metod.second)
      if (func.first == name.data())
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
