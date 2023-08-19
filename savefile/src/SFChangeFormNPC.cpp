#include "savefile/SFChangeFormNPC.h"

#include <sstream>

namespace {
enum Flags
{
  CHANGE_ACTOR_BASE_FACTIONS = 0x00000040,
  CHANGE_ACTOR_BASE_FULLNAME = 0x00000020,
  CHANGE_NPC_RACE = 0x02000000,
  CHANGE_NPC_GENDER = 0x01000000,
  CHANGE_NPC_FACE = 0x00000800
};

template <class T>
void Write(std::ostream& s, T v)
{
  s.write((char*)&v, sizeof(v));
}

void Write(std::ostream& s, std::string v)
{
  Write(s, uint16_t(v.size()));
  for (size_t i = 0; i < v.size(); ++i)
    Write(s, v.at(i));
}

void WriteVsval(std::ostream& s, const uint32_t& vsval)
{
  if (vsval <= 0x3F) { // value is uint8 _0x3f
    Write(s, static_cast<uint8_t>(vsval << 2));
  } else if (vsval <= 0x3FFF) { // value is uint16
    Write(s, static_cast<uint16_t>((vsval << 2) | 1));
  } else { // value is uint32
    Write(s, (vsval << 2) | 2);
  }
}
}

std::pair<uint32_t, std::vector<uint8_t>> SaveFile_::ChangeFormNPC_::ToBinary()
  const noexcept
{
  std::stringstream ss;
  uint32_t flags = 0;

  if (factions.has_value()) {

    WriteVsval(ss, factions->size() * ((sizeof(RefID) + sizeof(uint8_t))));

    for (auto& faction : *factions) {
      Write(ss, faction.facID);
      Write(ss, faction.factionRank);
    }
    flags |= CHANGE_ACTOR_BASE_FACTIONS;
  }

  if (playerName.has_value()) {
    Write(ss, *playerName);
    flags |= CHANGE_ACTOR_BASE_FULLNAME;
  }

  if (race.has_value()) {
    Write(ss, race->myRaceNow);
    Write(ss, race->defaultRace);
    flags |= CHANGE_NPC_RACE;
  }

  if (face.has_value()) {
    Write(ss, uint8_t(1));
    Write(ss, face->hairColorForm);
    Write(ss, face->bodySkinColor);
    Write(ss, face->headTextureSet);

    WriteVsval(ss, face->headParts.size());
    for (auto& hp : face->headParts)
      Write(ss, hp);

    Write(ss, uint8_t(1));

    Write(ss, face->options.size());
    for (auto& op : face->options)
      Write(ss, op);

    Write(ss, face->presets.size());
    for (auto& pr : face->presets)
      Write(ss, pr);

    flags |= CHANGE_NPC_FACE;
  }

  if (gender.has_value()) {
    Write(ss, *this->gender);
    flags |= CHANGE_NPC_GENDER;
  }

  auto s = ss.str();
  std::pair<uint32_t, std::vector<uint8_t>> result;
  result.first = flags;
  result.second.reserve(s.size());
  for (auto& v : s)
    result.second.push_back(v);
  return result;
}
