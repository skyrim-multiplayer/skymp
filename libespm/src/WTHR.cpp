#include "libespm/WTHR.h"
#include <fstream>
#include <iostream>

void parseWTHR(const std::string& filePath)
{
  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open())
  {
    std::cerr << "Failed to open file: " << filePath << std::endl;
    return;
  }

  WTHR wthr;

  std::getline(file, wthr.editorId, '\0');

  //Чтение тексур в зависимости от кол-ва слоев

  //file.read(reinterpret_cast<char*>(&wthr.unknownLNAM), sizeof(wthr.unknownLNAM));

  file.read(reinterpret_cast<char*>(&wthr.precipitation), sizeof(wthr.precipitation));

  file.read(reinterpret_cast<char*>(&wthr.visualEffect), sizeof(wthr.visualEffect));

  //file.read(reinterpret_cast<char*>(wthr.unknownRNAM.data()), sizeof(wthr.unknownRNAM));

  //file.read(reinterpret_cast<char*>(wthr.unknownQNAM.data()), sizeof(wthr.unknownQNAM));

  for (auto& color : wthr.cloudTextureColors)
  {
    file.read(reinterpret_cast<char*>(color.data()), sizeof(color));
  }

  for (auto& alpha : wthr.cloudTextureAlphas)
  {
    file.read(reinterpret_cast<char*>(alpha.data()), sizeof(alpha));
  }

  for (auto& color : wthr.unknownNAM0)
  {
    file.read(reinterpret_cast<char*>(color.data()), sizeof(color));
  }

  //file.read(reinterpret_cast<char*>(&wthr.fogDistance), sizeof(wthr.fogDistance));

  //file.read(reinterpret_cast<char*>(&wthr.data), sizeof(wthr.data));

  file.read(reinterpret_cast<char*>(&wthr.unknownNAM1), sizeof(wthr.unknownNAM1));

  //чтение звуков в зависимости от количества

  file.read(reinterpret_cast<char*>(&wthr.skyStatics), sizeof(wthr.skyStatics));

  file.read(reinterpret_cast<char*>(wthr.imageSpaces.data()), sizeof(wthr.imageSpaces));

  file.read(reinterpret_cast<char*>(&wthr.directionalAmbient), sizeof(wthr.directionalAmbient));

  std::getline(file, wthr.auroraModel, '\0');

  file.read(reinterpret_cast<char*>(wthr.unknownNAM2.data()), sizeof(wthr.unknownNAM2));

  file.read(reinterpret_cast<char*>(wthr.unknownNAM3.data()), sizeof(wthr.unknownNAM3));

  file.close();

}

