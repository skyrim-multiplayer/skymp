#pragma once
#include "CIString.h"
#include "Structures.h"
#include <MakeID.h-1.0.2>
#include <functional>
#include <map>
#include <set>

class VirtualMachine;

class StackIdHolder
{
public:
  StackIdHolder(VirtualMachine& vm_);
  ~StackIdHolder();
  int32_t GetStackId() const;

  StackIdHolder& operator=(const StackIdHolder&) = delete;
  StackIdHolder(const StackIdHolder&) = delete;

private:
  int32_t stackId;
  std::shared_ptr<MakeID> makeId;
};

struct VmExceptionInfo
{
  std::string what;
  std::string sourcePex;
};

class VirtualMachine
{
  friend class StackIdHolder;

public:
  using OnEnter = std::function<void(const StackIdHolder&)>;
  using ExceptionHandler = std::function<void(VmExceptionInfo)>;
  using MissingScriptHandler =
    std::function<std::optional<PexScript::Lazy>(std::string)>;

  struct ScriptInfo
  {
    std::string name;
    std::shared_ptr<IVariablesHolder> vars;
  };

  VirtualMachine(const std::vector<PexScript::Lazy>& loadedScripts);
  VirtualMachine(const std::vector<std::shared_ptr<PexScript>>& loadedScripts);

  void SetMissingScriptHandler(const MissingScriptHandler& handler);

  void SetExceptionHandler(const ExceptionHandler& handler);

  void AddObject(std::shared_ptr<IGameObject> self,
                 const std::vector<ScriptInfo>& scripts);

  void RemoveObject(std::shared_ptr<IGameObject> self); // ?

  void RegisterFunction(const std::string& className,
                        const std::string& functionName,
                        const FunctionType& type, const NativeFunction& fn);

  void SendEvent(std::shared_ptr<IGameObject> self, const char* eventName,
                 const std::vector<VarValue>& arguments,
                 OnEnter enter = nullptr);

  void SendEvent(ActivePexInstance* instance, const char* eventName,
                 const std::vector<VarValue>& arguments);

  VarValue CallMethod(IGameObject* self, const char* methodName,
                      std::vector<VarValue>& arguments,
                      std::shared_ptr<StackIdHolder> stackIdHolder = nullptr);

  VarValue CallStatic(const std::string& className,
                      const std::string& functionName,
                      std::vector<VarValue>& arguments,
                      std::shared_ptr<StackIdHolder> stackIdHolder = nullptr);

  PexScript::Lazy GetPexByName(const std::string& name);

  std::shared_ptr<ActivePexInstance> CreateActivePexInstance(
    const std::string& pexScriptName, VarValue activeInstanceOwner,
    const std::shared_ptr<IVariablesHolder>& mapForFillProperties,
    const std::string& childrenName);

  bool IsNativeFunctionByNameExisted(const std::string& name) const;

  ExceptionHandler GetExceptionHandler() const;

private:
  CIMap<PexScript::Lazy> allLoadedScripts;

  std::map<std::string, std::map<std::string, NativeFunction>> nativeFunctions,
    nativeStaticFunctions;

  std::map<std::string, std::shared_ptr<ActivePexInstance>>
    instancesForStaticCalls;

  std::set<std::shared_ptr<IGameObject>> gameObjectsHolder;

  MissingScriptHandler missingScriptHandler;
  ExceptionHandler handler;

  std::shared_ptr<MakeID> stackIdMaker;
};
