#pragma once
#include <RE/BSScript/IFunctionArguments.h>
#include <functional>
#include <variant>
#include <vector>

class VmFunctionArguments : public RE::BSScript::IFunctionArguments
{
public:
  typedef size_t (*GetNumArguments)(void* state);
  using GetNthArgument = std::function<RE::BSScript::Variable(size_t)>;

  VmFunctionArguments(GetNumArguments getNumArgs_, GetNthArgument getNthArg_,
                      void* state_)
    : getNumArgs(getNumArgs_)
    , getNthArg(getNthArg_)
    , state(state_)
  {
  }

  bool operator()(
    RE::BSScrapArray<RE::BSScript::Variable>& dest) const override
  {
    size_t n = getNumArgs(state);
    dest.resize(n);
    for (size_t i = 0; i < n; ++i)
      dest[i] = getNthArg(i);
    return true;
  }

private:
  void* const state;
  GetNumArguments getNumArgs;
  GetNthArgument getNthArg;
};