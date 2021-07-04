#include "OpcodesImplementation.h"

VarValue OpcodesImplementation::StrCat(const VarValue& s1, const VarValue& s2,
                                       StringTable&)
{

  std::string temp;

  for (auto param : { &s1, &s2 }) {
    auto str = CastToString(*param);
    temp += static_cast<const char*>(str);
  }

  return VarValue(temp);
}

void OpcodesImplementation::ArrayFindElement(VarValue& array, VarValue& result,
                                             VarValue& needValue,
                                             VarValue& startIndex)
{

  if (array.pArray == nullptr || (int)startIndex < 0 ||
      (int)startIndex >= array.pArray->size()) {
    result = VarValue(-1);
    return;
  }

  auto res = std::find(array.pArray->begin() + (int)startIndex,
                       array.pArray->end(), needValue);

  if (res != array.pArray->end()) {
    result = VarValue(static_cast<int32_t>(res - array.pArray->begin()));
  } else {
    result = VarValue(-1);
  }
}

void OpcodesImplementation::ArrayRFindElement(VarValue& array,
                                              VarValue& result,
                                              VarValue& needValue,
                                              VarValue& startIndex)
{
  if (array.pArray != nullptr) {

    int32_t indexForStart = array.pArray->size() - 1;

    if ((int)startIndex < -1)
      indexForStart = array.pArray->size() + (int)startIndex;

    if (indexForStart >= array.pArray->size() || indexForStart < 0) {
      result = VarValue(-1);
      return;
    }

    auto res =
      std::find(array.pArray->rbegin() + array.pArray->size() - indexForStart,
                array.pArray->rend(), needValue);
    if (res == array.pArray->rend()) {
      result = VarValue(-1);
    } else {
      result = VarValue(static_cast<int32_t>(array.pArray->rend() - res - 1));
    }
  } else {
    result = VarValue(-1);
    return;
  }
}
