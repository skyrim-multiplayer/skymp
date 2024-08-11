#include "libespm/SLGM.h"
#include "libespm/CompressedFieldsCache.h"
#include "libespm/RecordHeaderAccess.h"
#include <fstream>
#include <iostream>


void parseSLGM(const std::string& filePath)
{
  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << filePath << std::endl;
    return;
  }

  SLGM slgm;

  std::getline(file, slgm.editorID, '\0');

  file.read(reinterpret_cast<char*>(slgm.objectBounds.data()), sizeof(slgm.objectBounds));

  std::getline(file, slgm.itemName, '\0');

  std::getline(file, slgm.model, '\0');

  file.read(reinterpret_cast<char*>(&slgm.numKeywords), sizeof(slgm.numKeywords));

  slgm.keywords.resize(slgm.numKeywords);
  file.read(reinterpret_cast<char*>(slgm.keywords.data()), slgm.numKeywords * sizeof(uint32_t));

  file.read(reinterpret_cast<char*>(&slgm.currentSoul), sizeof(slgm.currentSoul));

  file.read(reinterpret_cast<char*>(&slgm.data.baseValue), sizeof(slgm.data.baseValue));

  file.read(reinterpret_cast<char*>(&slgm.data.weight), sizeof(slgm.data.weight));

  file.read(reinterpret_cast<char*>(&slgm.soulCapacity), sizeof(slgm.soulCapacity));

  file.read(reinterpret_cast<char*>(&slgm.filledGem), sizeof(slgm.filledGem));

  file.read(reinterpret_cast<char*>(&slgm.sound), sizeof(slgm.sound));

  file.close();
}

namespace espm {

SLGM::Data SLGM::GetData(CompressedFieldsCache& cache) const
{
  Data res;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "DATA", 4)) {
        res.weight = *reinterpret_cast<const float*>(data + 0x4);
      }
    },
    cache);
  return res;
}

}

