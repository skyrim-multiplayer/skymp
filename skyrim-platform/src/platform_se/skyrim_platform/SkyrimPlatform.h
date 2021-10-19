#pragma once
#include "CallNativeApi.h"  // CallNativeApi::NativeCallRequirements
#include <RE/BSCoreTypes.h> // RE::VMStackID
#include <RE/BSScript/IVirtualMachine.h>
#include <SKSE/API.h>
#include <exception>
#include <skse64/PluginAPI.h>
#include <stdexcept>

class SkyrimPlatform
{
public:
  static constexpr auto kPluginName = "SkyrimPlatform";
  static constexpr auto kPluginVersion = 0;

  static void ForceFirstTick();
  static void BeginMain();
  static void EndMain();
  static bool QuerySKSEPlugin(const SKSE::QueryInterface* skse,
                              SKSE::PluginInfo* info);
  static bool LoadSKSEPlugin(const SKSEInterface* skse);

  // Throws if we are not in OnUpdate context
  static const CallNativeApi::NativeCallRequirements&
  GetNativeCallRequirements();

  static void ExecuteInChakraThread(std::function<void(int)> f);

  static void SendException(std::exception_ptr exceptionPtr);
  static void SendException(const std::string& exception);

  static void AddUpdateTask(std::function<void()> f);
  static void AddTickTask(std::function<void()> f);

private:
  static void OnTick();
  static void OnUpdate(RE::BSScript::IVirtualMachine* vm,
                       RE::VMStackID stackId);
  static SkyrimPlatform& GetSingleton();

  SkyrimPlatform();

  void Tick();
  void Update(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId);

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};