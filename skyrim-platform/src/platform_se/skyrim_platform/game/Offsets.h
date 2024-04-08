#pragma once

/**
 * Idea is to keep offsets in one place so it would be easier to control these
 * in case of changes etc.
 */
namespace Offsets {

const uintptr_t BaseAddress = Offsets::BaseAddress;

namespace Hooks {
#ifdef SKYRIMSE
inline REL::Relocation<std::uintptr_t> VPrint{ REL::ID(50180), 0x163 };
inline REL::Relocation<std::uintptr_t> FrameUpdate{ REL::ID(35565), 0x53 };
inline REL::Relocation<std::uintptr_t> SendEvent{ REL::ID(98077) };
inline REL::Relocation<std::uintptr_t> DrawSheatheWeaponPC{ REL::ID(40232) };
inline REL::Relocation<std::uintptr_t> DrawSheatheWeaponActor{ REL::ID(
  36289) };
inline REL::Relocation<std::uintptr_t> SendAnimation{ REL::ID(37020) };
inline REL::Relocation<std::uintptr_t> QueueNinodeUpdate{ REL::ID(39181) };
inline REL::Relocation<std::uintptr_t> ApplyMasksToRenderTargets{ REL::ID(
  26454) };
inline REL::Relocation<std::uintptr_t> RenderCursorMenu{ REL::ID(32867) };
#else
inline REL::Relocation<std::uintptr_t> VPrint{ REL::ID(51110), 0x300 };
inline REL::Relocation<std::uintptr_t> FrameUpdate{ REL::ID(36564), 0x6e };
inline REL::Relocation<std::uintptr_t> SendEvent{ REL::ID(104800) };
inline REL::Relocation<std::uintptr_t> DrawSheatheWeaponPC{ REL::ID(41235) };
inline REL::Relocation<std::uintptr_t> DrawSheatheWeaponActor{ REL::ID(
  37279) };
inline REL::Relocation<std::uintptr_t> SendAnimation{ REL::ID(38048) };
inline REL::Relocation<std::uintptr_t> QueueNinodeUpdate{ REL::ID(40255) };
inline REL::Relocation<std::uintptr_t> ApplyMasksToRenderTargets{ REL::ID(
  27040) };
inline REL::Relocation<std::uintptr_t> RenderCursorMenu{ REL::ID(33632) };
#endif
}

namespace EventSource {
#ifdef SKYRIMSE
inline constexpr REL::ID ActorKill(37390);
inline constexpr REL::ID BooksRead(17470);
inline constexpr REL::ID CriticalHit(37726);
inline constexpr REL::ID DisarmedEvent(37392);
inline constexpr REL::ID DragonSoulsGained(37571);
inline constexpr REL::ID ItemHarvested(14704);
inline constexpr REL::ID LevelIncrease(39247);
inline constexpr REL::ID LocationDiscovery(40056);
inline constexpr REL::ID ShoutAttack(40060);
inline constexpr REL::ID SkillIncrease(39248);
inline constexpr REL::ID SoulsTrapped(37916);
inline constexpr REL::ID SpellsLearned(37917);
#else
inline constexpr REL::ID ActorKill(38338);
inline constexpr REL::ID BooksRead(17865);
inline constexpr REL::ID CriticalHit(38671);
inline constexpr REL::ID DisarmedEvent(38340);
inline constexpr REL::ID DragonSoulsGained(38520);
inline constexpr REL::ID ItemHarvested(14875);
inline constexpr REL::ID LevelIncrease(40319);
inline constexpr REL::ID LocationDiscovery(41067);
inline constexpr REL::ID ShoutAttack(41071);
inline constexpr REL::ID SkillIncrease(40320);
inline constexpr REL::ID SoulsTrapped(38873);
inline constexpr REL::ID SpellsLearned(38874);
#endif
}

namespace BSRenderManager {
#ifdef SKYRIMSE
inline constexpr REL::ID Singleton(524907);
#else
inline constexpr REL::ID Singleton(411393);
#endif
}

namespace MenuScreenData {
#ifdef SKYRIMSE
inline constexpr REL::ID Singleton(517043);
#else
inline constexpr REL::ID Singleton(403551);
#endif
}

#ifdef SKYRIMSE
inline constexpr REL::ID WinMain(35545);
#else
inline constexpr REL::ID WinMain(36544);
#endif

inline RE::ObjectRefHandle GetInvalidRefHandle()
{
#ifdef SKYRIMSE
  REL::Relocation<RE::ObjectRefHandle*> handle{ REL::ID(514164) };
#else
  REL::Relocation<RE::ObjectRefHandle*> handle{ REL::ID(400312) };
#endif
  return *handle;
}

/**
 * This is called from CallNative::CallNativeSafe
 * no idea what it does, should be renamed.
 */
inline float Unknown(void* unk1, void* unk2, RE::TESObjectREFR* obj)
{
  using func_t = decltype(&Unknown); // 55622
#ifdef SKYRIMSE
  REL::Relocation<func_t> func{ REL::ID(55622) };
#else
  REL::Relocation<func_t> func{ REL::ID(56151) };
#endif
  return func(unk1, unk2, obj);
}

inline void PushActorAway(void* vm, StackID stackId, RE::Actor* self,
                          RE::Actor* targetActor, float magnitude)
{
  using func_t = decltype(&PushActorAway);
#ifdef SKYRIMSE
  REL::Relocation<func_t> func{ REL::ID(55682) };
#else
  REL::Relocation<func_t> func{ REL::ID(56213) };
#endif
  func(vm, stackId, self, targetActor, magnitude);
}

}

class TESDataHandlerExtension
{
public:
  RE::ObjectRefHandle CreateReferenceAtLocationImpl(
    RE::TESBoundObject* a_base, const RE::NiPoint3& a_location,
    const RE::NiPoint3& a_rotation, RE::TESObjectCELL* a_targetCell,
    RE::TESWorldSpace* a_selfWorldSpace,
    RE::TESObjectREFR* a_alreadyCreatedRef, RE::BGSPrimitive* a_primitive,
    const RE::ObjectRefHandle& a_linkedRoomRefHandle, bool a_forcePersist,
    bool a_arg11)
  {
    using func_t =
      decltype(&TESDataHandlerExtension::CreateReferenceAtLocationImpl);

    int id;
#ifdef SKYRIMSE
    id = 13625;
#else
    id = 13723;
#endif

    REL::Relocation<func_t> func{ REL::ID(id) };
    return func(this, a_base, a_location, a_rotation, a_targetCell,
                a_selfWorldSpace, a_alreadyCreatedRef, a_primitive,
                a_linkedRoomRefHandle, a_forcePersist, a_arg11);
  }
};
