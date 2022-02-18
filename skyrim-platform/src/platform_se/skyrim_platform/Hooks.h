#pragma once

namespace Hooks {

template <class T>
void write_thunk_call(std::uintptr_t a_src)
{
  auto& trampoline = SKSE::GetTrampoline();
  T::func = trampoline.write_call<5>(a_src, T::thunk);
}

template <std::size_t idx, class T>
void write_vfunc(REL::Relocation<uintptr_t> vtbl)
{
  T::func = vtbl.write_vfunc(idx, T::thunk);
}

void Install();

}
