#include "libespm/SHOU.h"
#include <fstream>
#include <iostream>

void parseSHOU(const std::string& filePath)
{
  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << filePath << std::endl;
    return;
  }

  SHOU shou;

  std::getline(file, shou.editorId, '\0');

  std::getline(file, shou.fullName, '\0');

  file.read(reinterpret_cast<char*>(&shou.inventoryModel), sizeof(shou.inventoryModel));

  std::getline(file, shou.description, '\0');

  for (int i = 0; i < 3; ++i) {
    SHOU::ShoutData data;
    file.read(reinterpret_cast<char*>(&data.wordOfPower), sizeof(data.wordOfPower));
    file.read(reinterpret_cast<char*>(&data.spellEffect), sizeof(data.spellEffect));
    file.read(reinterpret_cast<char*>(&data.recoveryTime), sizeof(data.recoveryTime));
    shou.shoutData.push_back(data);
  }

  file.close();
}

