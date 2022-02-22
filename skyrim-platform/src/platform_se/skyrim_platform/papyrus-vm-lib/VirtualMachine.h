#pragma once
#include "Structures.h"

class VirtualMachine
{
  void RemoveObject(std::shared_ptr<IGameObject> self); // ?

public:
  std::map<std::shared_ptr<IGameObject>, std::vector<ActivePexInstance>>
    gameObjects;
  std::map<std::string, std::map<std::string, ::NativeFunction>>
    nativeFunctions, nativeStaticFunctions;

  std::vector<std::shared_ptr<PexScript>> allLoadedScripts;

  VirtualMachine(std::vector<std::shared_ptr<PexScript>> loadedScripts);

  void AddObject(std::shared_ptr<IGameObject> self,
                 std::vector<std::string> scripts, VarForBuildActivePex vars);

  void RegisterFunction(std::string className, std::string functionName,
                        FunctionType type, ::NativeFunction fn);

  void SendEvent(std::shared_ptr<IGameObject> self, const char* eventName,
                 std::vector<VarValue>& arguments);
  void SendEvent(ActivePexInstance* instance, const char* eventName,
                 std::vector<VarValue>& arguments);

  VarValue CallMethod(ActivePexInstance* instance, IGameObject* self,
                      const char* methodName,
                      std::vector<VarValue>& arguments);

  VarValue CallStatic(std::string className, std::string functionName,
                      std::vector<VarValue>& arguments);
};
