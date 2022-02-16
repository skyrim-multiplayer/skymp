#pragma once

namespace Offsets {

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
