#pragma once

namespace Offsets {
namespace BSRenderManager {
inline constexpr REL::ID Singleton(static_cast<std::uint64_t>(411393));
}

inline RE::ObjectRefHandle GetInvalidRefHandle()
{
  REL::Relocation<RE::ObjectRefHandle*> handle{ REL::ID(400312) };
  return *handle;
}

/**
 * This is called from CallNative::CallNativeSafe
 * no idea what it does, should be renamed.
 */
inline float Unknown(void* unk1, void* unk2, RE::TESObjectREFR* obj)
{
  using func_t = decltype(&Unknown);
  REL::Relocation<func_t> func{ REL::ID(56151) };
  return func(unk1, unk2, obj);
}

inline void PushActorAway(void* vm, StackID stackId, RE::Actor* self,
                          RE::Actor* targetActor, float magnitude)
{
}
}
