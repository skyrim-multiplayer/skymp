// Made for skymp2-server (June 2018)
// Thanks to Ivan Savelo for his help
#pragma once
#include <vector>

template <class T>
class DSLine
{
public:
  T& operator[](int64_t index)
  {
    const size_t realIdx = index >= 0 ? size_t(index) : size_t(-index);
    std::vector<T>& vec = index >= 0 ? positive : negative;
    if (vec.size() <= realIdx)
      vec.resize(realIdx + 1);
    return vec[realIdx];
  }

private:
  std::vector<T> positive; // >= 0
  std::vector<T> negative; // < 0
};