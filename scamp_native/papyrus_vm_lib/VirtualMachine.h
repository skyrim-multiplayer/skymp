#pragma once
#include "Structures.h"
#include <functional>
#include <map>

class VirtualMachine
{

public:
  VirtualMachine(std::vector<PexScript::Ptr> loadedScripts);

  void AddObject(IGameObject::Ptr self, std::vector<std::string> scripts,
                 PropertyValuesMap vars);

  void RemoveObject(IGameObject::Ptr self); // ?

  void RegisterFunction(std::string className, std::string functionName,
                        FunctionType type, NativeFunction fn);

  void SendEvent(IGameObject::Ptr self, const char* eventName,
                 std::vector<VarValue>& arguments);

  void SendEvent(ActivePexInstance* instance, const char* eventName,
                 std::vector<VarValue>& arguments);

  VarValue CallMethod(const std::string& ActiveInstanceName, VarValue* self,
                      const char* methodName,
                      std::vector<VarValue>& arguments);

  VarValue CallStatic(std::string className, std::string functionName,
                      std::vector<VarValue>& arguments);

  ActivePexInstance& GetActivePexInObject(VarValue* object,
                                          const std::string& scriptType);

  PexScript::Ptr GetPexByName(const std::string& name);

  ActivePexInstance::Ptr CreateActivePexInstance(
    const std::string& pexScriptName, VarValue activeInstanceOwner,
    VarForBuildActivePex mapForFillPropertys, std::string childrenName);

  std::map<std::string, std::map<std::string, NativeFunction>> nativeFunctions,
    nativeStaticFunctions;

protected:
  std::map<IGameObject::Ptr, std::vector<ActivePexInstance>> gameObjects;

  std::vector<PexScript::Ptr> allLoadedScripts;

private:
  using RegisteredGameOgject =
    std::pair<const IGameObject::Ptr, std::vector<ActivePexInstance>>;
};
