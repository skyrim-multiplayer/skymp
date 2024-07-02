#pragma once
#include <cstdint>
#include <vector>

// struct FunctionCode
// {
//   enum
//   {
//     kOp_Nop = 0,
//     kOp_IAdd,
//     kOp_FAdd,
//     kOp_ISubtract,
//     kOp_FSubtract, // 4
//     kOp_IMultiply,
//     kOp_FMultiply,
//     kOp_IDivide,
//     kOp_FDivide, // 8
//     kOp_IMod,
//     kOp_Not,
//     kOp_INegate,
//     kOp_FNegate, // C
//     kOp_Assign,
//     kOp_Cast,
//     kOp_CompareEQ,
//     kOp_CompareLT, // 10
//     kOp_CompareLTE,
//     kOp_CompareGT,
//     kOp_CompareGTE,
//     kOp_Jump, // 14
//     kOp_JumpT,
//     kOp_JumpF,
//     kOp_CallMethod,
//     kOp_CallParent, // 18
//     kOp_CallStatic,
//     kOp_Return,
//     kOp_Strcat,
//     kOp_PropGet, // 1C
//     kOp_PropSet,
//     kOp_ArrayCreate,
//     kOp_ArrayLength,
//     kOp_ArrayGetElement, // 20
//     kOp_ArraySetElement,
//     kOp_Invalid,
//   };

//   struct Instruction
//   {
//     uint8_t op = 0;
//     std::vector<VarValue> args;
//   };

//   std::vector<Instruction> instructions;
// };

class FunctionCodeView
{
public:
  FunctionCodeView(const uint8_t* data_, size_t dataSize_,
                   const StringTable* stringTable_) noexcept
  {
    data = data_;
    dataSize = dataSize_;
    stringTable = stringTable_;
  }

  enum
  {
    kOp_Nop = 0,
    kOp_IAdd,
    kOp_FAdd,
    kOp_ISubtract,
    kOp_FSubtract, // 4
    kOp_IMultiply,
    kOp_FMultiply,
    kOp_IDivide,
    kOp_FDivide, // 8
    kOp_IMod,
    kOp_Not,
    kOp_INegate,
    kOp_FNegate, // C
    kOp_Assign,
    kOp_Cast,
    kOp_CompareEQ,
    kOp_CompareLT, // 10
    kOp_CompareLTE,
    kOp_CompareGT,
    kOp_CompareGTE,
    kOp_Jump, // 14
    kOp_JumpT,
    kOp_JumpF,
    kOp_CallMethod,
    kOp_CallParent, // 18
    kOp_CallStatic,
    kOp_Return,
    kOp_Strcat,
    kOp_PropGet, // 1C
    kOp_PropSet,
    kOp_ArrayCreate,
    kOp_ArrayLength,
    kOp_ArrayGetElement, // 20
    kOp_ArraySetElement,
    kOp_Invalid,
  };

  template <class Callback /* void(const InstructionView&) */>
  void ForEachInstruction(const Callback& callback) const noexcept;

private:
  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  const StringTable* stringTable = nullptr;
};
