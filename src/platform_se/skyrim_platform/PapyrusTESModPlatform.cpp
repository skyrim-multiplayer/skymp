#include "PapyrusTESModPlatform.h"
#include "NullPointerException.h"
#include <RE/BSScript/IFunctionArguments.h>
#include <RE/BSScript/IStackCallbackFunctor.h>
#include <RE/BSScript/NativeFunction.h>
#include <RE/ConsoleLog.h>
#include <RE/SkyrimVM.h>

namespace TESModPlatform {
bool papyrusUpdateAllowed = false;
std::function<void(RE::BSScript::IVirtualMachine* vm, RE::VMStackID stackId)>
  onPapyrusUpdate = nullptr;
uint64_t numPapyrusUpdates = 0;

class FunctionArguments : public RE::BSScript::IFunctionArguments
{
public:
  bool operator()(
    RE::BSScrapArray<RE::BSScript::Variable>& a_dst) const override
  {
    a_dst.resize(0);
    return true;
  }
};

class StackCallbackFunctor : public RE::BSScript::IStackCallbackFunctor
{
public:
  void operator()(RE::BSScript::Variable a_result) override {}
  bool CanSave() const override { return false; }
  void SetObject(
    const RE::BSTSmartPointer<RE::BSScript::Object>& a_object) override{};
};
}

SInt32 TESModPlatform::Add(RE::BSScript::IVirtualMachine* vm,
                           RE::VMStackID stackId, RE::StaticFunctionTag*)
{
  if (!papyrusUpdateAllowed)
    return 0;
  papyrusUpdateAllowed = false;

  try {
    ++numPapyrusUpdates;
    if (!onPapyrusUpdate)
      throw NullPointerException("onPapyrusUpdate");
    onPapyrusUpdate(vm, stackId);

  } catch (std::exception& e) {
    if (auto console = RE::ConsoleLog::GetSingleton())
      console->Print("Papyrus context exception: %s", e.what());
  }

  return 0;
}

void TESModPlatform::Update()
{
  papyrusUpdateAllowed = true;

  auto console = RE::ConsoleLog::GetSingleton();
  if (!console)
    return;

  auto vm = RE::SkyrimVM::GetSingleton();
  if (!vm || !vm->impl)
    return console->Print("VM was nullptr");

  FunctionArguments args;
  RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> functor(
    new StackCallbackFunctor);

  RE::BSFixedString className("TESModPlatform");
  RE::BSFixedString funcName("Add");
  vm->impl->DispatchStaticCall(className, funcName, &args, functor);
}

uint64_t TESModPlatform::GetNumPapyrusUpdates()
{
  return numPapyrusUpdates;
}

bool TESModPlatform::Register(RE::BSScript::IVirtualMachine* vm)
{
  TESModPlatform::onPapyrusUpdate = onPapyrusUpdate;

  vm->BindNativeMethod(
    new RE::BSScript::NativeFunction<true, decltype(Add), SInt32,
                                     RE::StaticFunctionTag*>(
      "Add", "TESModPlatform", Add));
  return true;
}