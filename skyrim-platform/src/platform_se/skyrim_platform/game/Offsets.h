#pragma once

/**
 * Idea is to keep offsets in one place so it would be easier to control these
 * in case of changes etc.
 */
namespace Offsets {

const uintptr_t BaseAddress = Offsets::BaseAddress;

namespace Hooks {
inline REL::Relocation<std::uintptr_t> VPrint{
  RELOCATION_ID(50180, 51110), REL::VariantOffset(0x163, 0x300, 0x163)
};
inline REL::Relocation<std::uintptr_t> FrameUpdate{
  RELOCATION_ID(35565, 36564), REL::VariantOffset(0x53, 0x6e, 0x68)
};
inline REL::Relocation<std::uintptr_t> SendEvent{ RELOCATION_ID(98077,
                                                                104800) };
inline REL::Relocation<std::uintptr_t> DrawSheatheWeaponPC{ RELOCATION_ID(
  40232, 41235) };
inline REL::Relocation<std::uintptr_t> DrawSheatheWeaponActor{ RELOCATION_ID(
  36289, 37279) };
inline REL::Relocation<std::uintptr_t> SendAnimation{ RELOCATION_ID(37020,
                                                                    38048) };
inline REL::Relocation<std::uintptr_t> QueueNinodeUpdate{ RELOCATION_ID(
  39181, 40255) };
inline REL::Relocation<std::uintptr_t> ApplyMasksToRenderTargets{
  RELOCATION_ID(26454, 27040)
};
inline REL::Relocation<std::uintptr_t> RenderCursorMenu{ RELOCATION_ID(
  32867, 33632) };
}

namespace EventSource {
inline constexpr REL::ID ActorKill(REL::Relocate(37390, 38338));
inline constexpr REL::ID BooksRead(REL::Relocate(17470, 17865));
inline constexpr REL::ID CriticalHit(REL::Relocate(37726, 38671));
inline constexpr REL::ID DisarmedEvent(REL::Relocate(37392, 38340));
inline constexpr REL::ID DragonSoulsGained(REL::Relocate(37571, 38520));
inline constexpr REL::ID ItemHarvested(REL::Relocate(14704, 14875));
inline constexpr REL::ID LevelIncrease(REL::Relocate(39247, 40319));
inline constexpr REL::ID LocationDiscovery(REL::Relocate(40056, 41067));
inline constexpr REL::ID ShoutAttack(REL::Relocate(40060, 41071));
inline constexpr REL::ID SkillIncrease(REL::Relocate(39248, 40320));
inline constexpr REL::ID SoulsTrapped(REL::Relocate(37916, 38873));
inline constexpr REL::ID SpellsLearned(REL::Relocate(37917, 38874));
}

namespace BSRenderManager {
inline constexpr REL::ID Singleton(REL::Relocate(524907, 411393));
}

namespace MenuScreenData {
inline constexpr REL::ID Singleton(REL::Relocate(517043, 403551));
}

inline constexpr REL::ID WinMain(REL::Relocate(35545, 36544));

inline RE::ObjectRefHandle GetInvalidRefHandle()
{
  REL::Relocation<RE::ObjectRefHandle*> handle{ REL::ID(
    REL::Relocate(514164, 400312)) };
  return *handle;
}

/**
 * This is called from CallNative::CallNativeSafe
 * no idea what it does, should be renamed.
 */
inline float Unknown(void* unk1, void* unk2, RE::TESObjectREFR* obj)
{
  using func_t = decltype(&Unknown); // 55622
  REL::Relocation<func_t> func{ REL::ID(REL::Relocate(55622, 56151)) };
  return func(unk1, unk2, obj);
}

inline void PushActorAway(void* vm, StackID stackId, RE::Actor* self,
                          RE::Actor* targetActor, float magnitude)
{
  using func_t = decltype(&PushActorAway);
  REL::Relocation<func_t> func{ REL::ID(REL::Relocate(55682, 56213)) };
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

    int id = REL::Relocate(13625, 13723);

    REL::Relocation<func_t> func{ REL::ID(id) };
    return func(this, a_base, a_location, a_rotation, a_targetCell,
                a_selfWorldSpace, a_alreadyCreatedRef, a_primitive,
                a_linkedRoomRefHandle, a_forcePersist, a_arg11);
  }
};
