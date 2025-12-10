#pragma once
#include "ConditionFunction.h"

namespace ConditionFunctions {

/*

For body part flags see:
https://en.uesp.net/wiki/Skyrim_Mod:Mod_File_Format/BOD2_Field

// No mask. Calculate all worn items
{
  "function": "SkympWornHasKeywordCount",
  "parameter2": "0x0",
  ...
},

// All mask bits set. Calculate all Armor
{
  "function": "SkympWornHasKeywordCount",
  "parameter2": "0b11111111111111111111111111111111",
  ...
},

// Calculate Armor with body part flags: Head, Hair and Hands
{
  "function": "SkympWornHasKeywordCount",
  "parameter2": "0b00000000000000000000000000001011",
  ...
}

*/
class SkympWornHasKeywordCount : public ConditionFunction
{
public:
  const char* GetName() const override;

  uint16_t GetFunctionIndex() const override;

  float Execute(MpActor& actor, uint32_t parameter1, uint32_t parameter2,
                const ConditionEvaluatorContext& context) override;
};
}
