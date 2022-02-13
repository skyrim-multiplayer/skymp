#include "Hooks.h"
#include "EventHandler.h"

ptrdiff_t g_ActiveEffectListSize = 0;
size_t g_newEffectListSize = 0;

struct OnUpdate
{
  static void thunk()
  {
    func();

    auto c = RE::ConsoleLog::GetSingleton();
    c->Print("update hook");

    auto pc = RE::PlayerCharacter::GetSingleton();
    if (!pc) {
      return;
    }

    auto effects = std::make_unique<RE::BSSimpleList<RE::ActiveEffect*>>(
      *pc->GetActiveEffectList());

    auto newEffectsSize = std::distance(effects->begin(), effects->end());

    if (g_ActiveEffectListSize == 0 ||
        g_ActiveEffectListSize != newEffectsSize) {

      g_ActiveEffectListSize = newEffectsSize;

      auto newList = std::make_unique<std::vector<RE::ActiveEffect*>>();

      std::copy_if(effects->begin(), effects->end(),
                   std::back_inserter(*newList.get()),
                   [](RE::ActiveEffect* eff) { return eff->usUniqueID != 0; });

      if (!newList->empty() || newList->size() != g_newEffectListSize) {
        logger::info("Effect list size changed. New list:");

        g_newEffectListSize = newList->size();

        for (const auto& effect : *newList.get()) {
          auto caster = effect->GetCasterActor()->formID;
          auto target = effect->GetTargetActor()->formID;
          auto duration = effect->duration;
          auto uid = effect->usUniqueID;
          auto baseId = effect->GetBaseObject()->formID;
          auto baseName = effect->GetBaseObject()->GetName();

          logger::info("uid - {}, baseName - {}, baseId - {}, duration - {}, "
                       "castedId - {}, targetId - {}",
                       uid, baseName, baseId, duration, caster, target);
        }
      }
    }
  };
  static inline REL::Relocation<decltype(&thunk)> func;
};

void InstallOnUpdateHook()
{
  REL::Relocation<std::uintptr_t> hook{ REL::ID(36564), 0x6e };
  Hooks::write_thunk_call<OnUpdate>(hook.address());
}

struct OnConsoleVPrint
{
  static void thunk(void* unk1, const char* msg)
  {
    if (msg) {
      EventHandler::SendEventConsoleMsg(msg);
    }

    func(unk1, msg);
  };
  static inline REL::Relocation<decltype(&thunk)> func;
};

void InstallOnConsoleVPrintHook()
{
  REL::Relocation<std::uintptr_t> hook{ REL::ID(51110), 0x300 };
  Hooks::write_thunk_call<OnConsoleVPrint>(hook.address());
}

void Hooks::Install()
{
  InstallOnUpdateHook();
  InstallOnConsoleVPrintHook();

  logger::info("CommonLib hooks installed.");
}
