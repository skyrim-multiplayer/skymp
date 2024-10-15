#pragma once

template <class T>
struct hkbVariableValueSet
{
  virtual ~hkbVariableValueSet();

  void* unk;
  T* varSet;
  uint32_t size;
};

static_assert(offsetof(hkbVariableValueSet<int>, varSet) == 0x10);
static_assert(offsetof(hkbVariableValueSet<int>, size) == 0x18);
