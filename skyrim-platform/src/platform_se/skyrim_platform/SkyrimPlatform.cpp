#include "SkyrimPlatform.h"

#include "BrowserApi.h"    // BrowserApi::State
#include "CallNativeApi.h" // CallNativeApi::NativeCallRequirements
#include "ConsoleApi.h"    // ConsoleApi::GetExceptionPrefix
#include "DumpFunctions.h"
#include "ExceptionPrinter.h"
#include "PapyrusTESModPlatform.h"
#include "ThreadPoolWrapper.h"
#include "TickTask.h"
#include <RE/ConsoleLog.h>
#include <SKSE/API.h>
#include <SKSE/Interfaces.h>
#include <SKSE/Stubs.h>
#include <skse64/PluginAPI.h>

// Listeners
#include "ExecutionCommonsListener.h"
#include "HelloListener.h"

void SetupFridaHooks();

namespace {
void PrintExceptionToGameConsole(const std::exception& e)
{
  if (auto console = RE::ConsoleLog::GetSingleton()) {
    std::string what = e.what();

    while (what.size() > sizeof("Error: ") - 1 &&
           !memcmp(what.data(), "Error: ", sizeof("Error: ") - 1)) {
      what = { what.begin() + sizeof("Error: ") - 1, what.end() };
    }
    ExceptionPrinter(ConsoleApi::GetExceptionPrefix())
      .PrintException(what.data());
  }
}

void UpdateDumpFunctions()
{
  auto pressed = [](int key) {
    return (GetAsyncKeyState(key) & 0x80000000) > 0;
  };
  const bool comb = pressed('9') && pressed('O') && pressed('L');
  static bool g_combWas = false;

  if (comb != g_combWas) {
    g_combWas = comb;
    if (comb) {
      DumpFunctions::Run();
    }
  }
}
}

struct SkyrimPlatform::Impl
{
  ThreadPoolWrapper pool;

  SKSETaskInterface* taskInterface = nullptr;
  SKSEMessagingInterface* messaging = nullptr;

  std::vector<std::shared_ptr<SkyrimPlatformListener>> listeners;

  std::shared_ptr<CallNativeApi::NativeCallRequirements>
    nativeCallRequirements;
};

void SkyrimPlatform::ForceFirstTick()
{
  OnTick();
}

void SkyrimPlatform::BeginMain()
{
  for (auto& listener : GetSingleton().pImpl->listeners) {
    listener->BeginMain();
  }
}

bool SkyrimPlatform::QuerySKSEPlugin(const SKSE::QueryInterface* skse,
                                     SKSE::PluginInfo* info)
{
  info->infoVersion = SKSE::PluginInfo::kVersion;
  info->name = kPluginName;
  info->version = kPluginVersion;

  if (skse->IsEditor()) {
    _FATALERROR("loaded in editor, marking as incompatible");
    return false;
  }
  return true;
}

bool SkyrimPlatform::LoadSKSEPlugin(const SKSEInterface* skse)
{
  auto& pImpl = GetSingleton().pImpl;

  pImpl->messaging = reinterpret_cast<SKSEMessagingInterface*>(
    skse->QueryInterface(kInterface_Messaging));
  if (!pImpl->messaging) {
    _FATALERROR("couldn't get messaging interface");
    return false;
  }
  pImpl->taskInterface = reinterpret_cast<SKSETaskInterface*>(
    skse->QueryInterface(kInterface_Task));
  if (!pImpl->taskInterface) {
    _FATALERROR("couldn't get task interface");
    return false;
  }

  auto papyrusInterface = static_cast<SKSEPapyrusInterface*>(
    skse->QueryInterface(kInterface_Papyrus));
  if (!papyrusInterface) {
    _FATALERROR("QueryInterface failed for PapyrusInterface");
    return false;
  }

  SetupFridaHooks();

  pImpl->taskInterface->AddTask(new TickTask(pImpl->taskInterface, OnTick));

  papyrusInterface->Register(
    reinterpret_cast<SKSEPapyrusInterface::RegisterFunctions>(
      TESModPlatform::Register));
  TESModPlatform::onPapyrusUpdate = OnUpdate;

  return true;
}

SkyrimPlatform::SkyrimPlatform()
{
  pImpl = std::make_shared<Impl>();
  pImpl->nativeCallRequirements =
    std::make_shared<CallNativeApi::NativeCallRequirements>();

  pImpl->listeners.push_back(std::make_shared<HelloListener>());
  pImpl->listeners.push_back(
    std::make_shared<ExecutionCommonsListener>(pImpl->nativeCallRequirements));
}

void SkyrimPlatform::Tick()
{
  for (auto& listener : pImpl->listeners) {
    try {
      listener->Tick();
    } catch (const std::exception& e) {
      PrintExceptionToGameConsole(e);
    }
  }
}

void SkyrimPlatform::Update(RE::BSScript::IVirtualMachine* vm,
                            RE::VMStackID stackId)
{
  pImpl->nativeCallRequirements->stackId = stackId;
  pImpl->nativeCallRequirements->vm = vm;

  for (auto& listener : pImpl->listeners) {
    try {
      listener->Update();
    } catch (const std::exception& e) {
      PrintExceptionToGameConsole(e);
    }
  }

  pImpl->nativeCallRequirements->gameThrQ->Update();
  pImpl->nativeCallRequirements->stackId = static_cast<RE::VMStackID>(~0);
  pImpl->nativeCallRequirements->vm = nullptr;
}

SkyrimPlatform& SkyrimPlatform::GetSingleton()
{
  static SkyrimPlatform g_skyrimPlatform;
  return g_skyrimPlatform;
}

void SkyrimPlatform::OnTick()
{
  auto& pool = GetSingleton().pImpl->pool;
  pool.PushAndWait([=](int) { GetSingleton().Tick(); });
  TESModPlatform::Update();
}

void SkyrimPlatform::OnUpdate(RE::BSScript::IVirtualMachine* vm,
                              RE::VMStackID stackId)
{
  UpdateDumpFunctions();

  auto& pool = GetSingleton().pImpl->pool;
  pool.PushAndWait([vm, stackId](int) { GetSingleton().Update(vm, stackId); });
}