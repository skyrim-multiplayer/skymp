#pragma once
#include <cstdint>

class MpActor;
struct ConditionEvaluatorContext;

class ConditionFunction
{
public:
  virtual ~ConditionFunction() = default;

  virtual const char* GetName() const = 0;

  // https://en.uesp.net/wiki/Skyrim_Mod:Function_Indices
  virtual uint16_t GetFunctionIndex() const = 0;

  virtual float Execute(MpActor& actor, uint32_t parameter1,
                        uint32_t parameter2,
                        const ConditionEvaluatorContext& context) = 0;
};
