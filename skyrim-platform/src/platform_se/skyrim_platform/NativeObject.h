#pragma once
#include "CallNative.h"

class NativeObject
{
public:
  NativeObject(const CallNative::ObjectPtr& obj_);
  const CallNative::ObjectPtr& Get() const;

  uint64_t papyrusUpdateId;
  CallNative::ObjectPtr obj;
};
