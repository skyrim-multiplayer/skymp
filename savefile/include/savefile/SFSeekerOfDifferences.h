#pragma once
#include "SFStructure.h"

#include <array>
#include <fstream>
#include <memory>
#include <vector>

namespace SaveFile_ {
class SeekerOfDifferences
{
private:
  struct Data
  {
    uint32_t changeFlags = 0;
    std::vector<uint8_t> value;
  };

  using ComparisonDifferences = std::vector<std::array<Data, 2>>;

  ComparisonDifferences StartCompare();

  template <class T>
  void Write(const T& data)
  {
    writer.write((char*)&data, sizeof(data));
  };

  std::shared_ptr<SaveFile> firstObject;
  std::shared_ptr<SaveFile> secondObject;

public:
  static void ZlibDecompress(const uint8_t* in, size_t inSize, uint8_t* out,
                             size_t outSize);
  static size_t ZlibCompress(const uint8_t* in, size_t inSize, uint8_t* out,
                             size_t outMaxSize);
  std::ofstream writer;
  void CoutVector(std::vector<uint8_t> vector, std::string nameObject);

  void AddToComparisonFirst(std::shared_ptr<SaveFile> firstObject)
  {
    this->firstObject = firstObject;
  };
  void AddToComparisonSecond(std::shared_ptr<SaveFile> secondObject)
  {
    this->secondObject = secondObject;
  };
  ComparisonDifferences CompareAddedObjects() { return StartCompare(); };
};
}
