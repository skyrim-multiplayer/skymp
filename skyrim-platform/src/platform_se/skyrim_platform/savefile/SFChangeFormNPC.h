#pragma once
#include "SFStructure.h"

namespace SaveFile_ {
struct ChangeFormNPC_
{
  struct FactionData
  {
    RefID facID;
    int8_t factionRank;
  };

  std::optional<std::vector<FactionData>> factions;

  std::optional<std::string> playerName;

  struct RaceChange
  {
    RefID myRaceNow;
    RefID defaultRace;
  };

  std::optional<RaceChange> race;

  struct Face
  {
    RefID hairColorForm;
    uint32_t bodySkinColor;
    RefID headTextureSet;
    std::vector<RefID> headParts;
    std::vector<float> options;
    std::vector<uint32_t> presets;
  };

  std::optional<Face> face;
  std::optional<uint8_t> gender;

  std::pair<uint32_t, std::vector<uint8_t>> ToBinary() const noexcept;
};
}
