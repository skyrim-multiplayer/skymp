#include "OpcodesImplementation.h"

void OpcodesImplementation::strCat(VarValue& result, VarValue& s1,
                                   VarValue& s2, StringTable& table)
{

  std::string temp;
  temp = temp + (const char*)s1 + (const char*)s2;

  for (auto& str : table.m_data) {
    if (str == temp) {
      result = VarValue(str.data());
      return;
    }
  }

  size_t _size = table.m_data.size();
  table.m_data.push_back(temp);
  result = VarValue(table.m_data[_size].data());
}

void OpcodesImplementation::arrayFindElement(VarValue& array, VarValue& result,
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

void OpcodesImplementation::arrayRFindElement(VarValue& array,
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
