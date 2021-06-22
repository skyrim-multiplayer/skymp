#pragma once

class ScopedTask
{
public:
  using Callback = void (*)(void*);

  ScopedTask(Callback f_, void* state_)
    : f(f_)
    , state(state_)
  {
  }

  ~ScopedTask() { f(state); }

private:
  const Callback f;
  void* const state;
};