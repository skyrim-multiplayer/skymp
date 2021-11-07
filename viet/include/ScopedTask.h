#pragma once

namespace Viet {
template <class State>
class ScopedTask
{
public:
  using Callback = void (*)(State&);

  ScopedTask(Callback f_, State& state_)
    : f(f_)
    , state(state_)
  {
  }

  ~ScopedTask() { f(state); }

private:
  const Callback f;
  State& state;
};
}
