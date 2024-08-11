#include "libespm/WOOP.h"
#include <fstream>
#include <iostream>

void parseWOOP(const std::string& filePath)
{
  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << filePath << std::endl;
    return;
  }

  WOOP woop;

  std::getline(file, woop.editorId, '\0');

  std::getline(file, woop.recordName, '\0');

  std::getline(file, woop.translation, '\0');

  file.close();
}

