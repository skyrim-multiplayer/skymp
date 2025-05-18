#pragma once
#include "MpActor.h"
#include <cstdint>

class ConditionFunction
{
public:
  virtual ~ConditionFunction() = default;

  virtual const char* GetName() const = 0;

  virtual uint16_t GetFunctionIndex() const = 0;

  virtual float Execute(MpActor& actor, uint32_t parameter1,
                        uint32_t parameter2) = 0;
};
