#pragma once

#include <string>
#include <vector>

struct SHOU
{  
  std::string editorId;
  std::string fullName;
  uint32_t inventoryModel;
  std::string description;

  struct ShoutData
  {
    uint32_t wordOfPower;
    uint32_t spellEffect;
    float recoveryTime;
  };

  std::vector<ShoutData> shoutData;
};

