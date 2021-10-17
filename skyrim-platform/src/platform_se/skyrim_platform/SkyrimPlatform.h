#pragma once
#include <RE/BSCoreTypes.h> // RE::VMStackID
#include <RE/BSScript/IVirtualMachine.h>
#include <SKSE/API.h>
#include <skse64/PluginAPI.h>

class SkyrimPlatform
{
public:
  static constexpr auto kPluginName = "SkyrimPlatform";
  static constexpr auto kPluginVersion = 0;

  static void ForceFirstTick();
  static void BeginMain();
  static bool QuerySKSEPlugin(const SKSE::QueryInterface* skse,
                              SKSE::PluginInfo* info);
  static bool LoadSKSEPlugin(const SKSEInterface* skse);

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