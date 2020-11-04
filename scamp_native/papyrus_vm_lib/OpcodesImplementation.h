#pragma once
#include "Structures.h"

namespace OpcodesImplementation {

enum Opcodes
{
  op_Nop = 0x00,
  op_iAdd = 0x01,
  op_fAdd = 0x02,
  op_iSub = 0x03,
  op_fSub = 0x04,
  op_iMul = 0x05,
  op_fMul = 0x06,
  op_iDiv = 0x07,
  op_fDiv = 0x08,
  op_iMod = 0x09,
  op_Not = 0x0A,
  op_iNeg = 0x0B,
  op_fNeg = 0x0C,
  op_Assign = 0x0D,
  op_Cast = 0x0E,
  op_Cmp_eq = 0x0F,
  op_Cmp_lt = 0x10,
  op_Cmp_le = 0x11,
  op_Cmp_gt = 0x12,
  op_Cmp_ge = 0x13,
  op_Jmp = 0x14,
  op_Jmpt = 0x15,
  op_Jmpf = 0x16,
  op_CallMethod = 0x17,
  op_CallParent = 0x18,
  op_CallStatic = 0x19,
  op_Return = 0x1A,
  op_StrCat = 0x1B,
  op_PropGet = 0x1C,
  op_PropSet = 0x1D,
  op_Array_Create = 0x1E,
  op_Array_Length = 0x1F,
  op_Array_GetElement = 0x20,
  op_Array_SetElement = 0x21,
  op_Array_FindElement = 0x22,
  op_Array_RfindElement = 0x23
};

VarValue StrCat(const VarValue& s1, const VarValue& s2, StringTable& table);

void ArrayFindElement(VarValue& array, VarValue& result, VarValue& needValue,
                      VarValue& startIndex);
void ArrayRFindElement(VarValue& array, VarValue& result, VarValue& needValue,
                       VarValue& startIndex);
}