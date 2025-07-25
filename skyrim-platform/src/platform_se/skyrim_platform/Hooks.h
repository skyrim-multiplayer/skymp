#pragma once
#include <utility>
#include <vector>

namespace Hooks {

template <class T>
void write_thunk_call(std::uintptr_t a_src)
{
  auto& trampoline = SKSE::GetTrampoline();
  T::func = trampoline.write_call<5>(a_src, T::thunk);
}

void Install();

std::vector<std::tuple<std::string, std::string, RE::BSScript::IFunction*,
                       uintptr_t, uintptr_t, uintptr_t>>
GetBoundNatives();

}
