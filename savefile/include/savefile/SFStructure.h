#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace SaveFile_ {

class SaveFile;

namespace {
const size_t numCrimesType = 7;
}

struct Unknown3Table
{
  uint32_t count;
  std::vector<std::string> unknown; // unknown[count]
};

struct RefID
{ // The lower 22 bits represent the formID value itself, while the upper 2
  // bits are the type of formID.
  uint8_t byte0 = 0; // The upper two bits represent the type of formID:
                     // 0 = An index into the File.formIDArray. If the index
                     // value of 0 is given, the formID is 0x00000000, else,
                     // index into the array using value - 1. 1 = Default (ie,
                     // came from Skyrim.esm) 2 = Created (ie, plugin index of
                     // 0xFF) 3 = ???
  uint8_t byte1 = 0;
  uint8_t byte2 = 0;

  RefID(){};

  enum
  {
    PlayerBase = 0x7,
    Player = 0x14
  };

  RefID(int refID)
  {
    uint8_t* values = reinterpret_cast<uint8_t*>(&refID);

    this->byte0 = values[2];
    this->byte1 = values[1];
    this->byte2 = values[0];

    if (values[3] == 0x00)
      this->byte0 |= 0b01000000;
  }

  RefID(uint32_t refID)
  {
    uint8_t* values = reinterpret_cast<uint8_t*>(&refID);

    this->byte0 = values[2];
    this->byte1 = values[1];
    this->byte2 = values[0];
  }

  static RefID CreateRefId(SaveFile& parentSaveFile, uint32_t formId);

  bool IsPlayerID()
  {
    return ((this->byte0 == 0x40) && (this->byte1 == 0x00) &&
            (this->byte2 == 0x14));
  }

  bool IsPlayerBaseID()
  {
    return ((this->byte0 == 0x40) && (this->byte1 == 0x00) &&
            (this->byte2 == 0x7));
  }

  bool operator==(RefID& objToCompare)
  {
    return ((this->byte0 == objToCompare.byte0) &&
            (this->byte1 == objToCompare.byte1) &&
            (this->byte2 == objToCompare.byte2));
  }

  bool operator!=(RefID& objToCompare)
  {
    return ((this->byte0 != objToCompare.byte0) ||
            (this->byte1 != objToCompare.byte1) ||
            (this->byte2 != objToCompare.byte2));
  }
};

struct ChangeForm
{
  enum class Type
  {
    NPC = 9,
    ACHR = 1,
    VersionSkyrim_1_9 = 74
  };

  RefID formID;
  uint32_t changeFlags;
  uint8_t type;
  uint8_t version; // Current as of Skyrim 1.9 is 74. Older values (57, 64, 73)
                   // are also valid
  uint32_t length1;          // actual type depends on flags
  uint32_t length2;          // actual type depends on flags. If this value is
                             // non-zero, data is compressed. This value then
                             // represents the uncompressed length.
  std::vector<uint8_t> data; // data[length1]

  bool Is_NPC_Type() { return ((this->type & 0b00111111) == 9); };
  bool Is_ACHR_Type() { return ((this->type & 0b00111111) == 1); };
};

struct GlobalData
{
  uint32_t type;
  uint32_t length;
  std::shared_ptr<void> data;
};

struct FileLocationTable
{
  uint32_t formIDArrayCountOffset;
  uint32_t unknownTable3Offset;
  uint32_t globalDataTable1Offset;
  uint32_t globalDataTable2Offset;
  uint32_t changeFormsOffset;
  uint32_t globalDataTable3Offset;
  uint32_t globalDataTable1Count;
  uint32_t globalDataTable2Count;
  uint32_t globalDataTable3Count;
  uint32_t changeFormCount;
  uint32_t unused[15];
};

struct PluginInfo
{
  uint8_t numPlugins;
  std::vector<std::string> pluginsName; // plugins[pluginCount]
};

struct Header
{
  uint32_t version;
  uint32_t saveNumber;
  std::string playerName;
  uint32_t playerLevel;
  std::string playerLocation;
  std::string gameDate;
  std::string playerRaceEditorId;
  uint16_t playerSex;   // 0 - male, 1 - female
  float playerCurExp;   // 32 bit
  float playerLvlUpExp; // 32 bit
  uint8_t filetime[8];
  uint32_t shotWidth;  // screenshot width in pixels
  uint32_t shotHeight; // screenshot height in pixels
};

struct GlobalVariables
{ // global data type=3
  struct GlobalVariable
  {
    RefID formID;
    float value;
  };

  uint32_t numGlobals; // vsval
  std::vector<GlobalVariable> globals;
};

struct SaveFile
{
  enum
  {
    GLOBAL_VARIABLES_INDEX = 3,
    WEATHER_INDEX = 6,
  };

  std::string magic; // Constant: "TESV_SAVEGAME"
  uint32_t headerSize;
  Header header;
  std::vector<uint8_t> screenshotData; //[3*header.shotWidth*header.showHeight]
                                       //-> pixel data in RGB
  uint8_t formVersion;                 // current as of Skyrim 1.9 is 74
  uint32_t pluginInfoSize;
  PluginInfo pluginInfo;
  FileLocationTable fileLocationTable;
  std::vector<GlobalData> globalDataTable1; // Types 0 to 8
  std::vector<GlobalData> globalDataTable2; // Types 100 to 114
  std::vector<ChangeForm> changeForms;
  std::vector<GlobalData> globalDataTable3; // Types 1000 to 1005
  uint64_t fixForBag; // globalDataTable3Count is currently bugged
  uint32_t formIDArrayCount;
  std::vector<uint32_t> formIDArray;

  uint32_t visitedWorldspaceArrayCount;
  std::vector<uint32_t> visitedWorldspaceArray;

  uint32_t unknown3TableSize;
  Unknown3Table unknown3Table;

  ChangeForm* GetChangeFormByRefID(RefID refID, const uint8_t& type);
  GlobalVariables::GlobalVariable* GetGlobalvariableByRefID(RefID& refID);
  int64_t FindIndexInFormIdArray(uint32_t refID);
  void OverwritePluginInfo(std::vector<std::string>& newPlaginNames);
};

struct MiscStats
{ // global data type=0
  struct MiscStat
  {
    std::string name;
    uint8_t category;

    enum MiscStatCattegiries
    {
      General = 0,
      Quest,
      Combat,
      Magic,
      Crafting,
      Crime,
      DLC_Stats /// Showed up in Skyrim 1.7.7,
                /// but nothing shows up in the in - game menu.
                /// Four values observed : NumVampirePerks, NumWerewolfPerks(if
                /// Dawnguard is installed), SolstheimLocationsDiscovered and
                /// StalhrimItemsCrafted(if Dragonborn is installed).
    };

    int32_t value;
  };

  uint32_t numStats;
  std::vector<MiscStat> stats;
};

struct PlayerLocation
{
  enum
  {
    GlobalDataType = 1
  };
  uint32_t
    nextObjectId; // Number of next savegame specific object id, i.e. FFxxxxxx.
  RefID worldspace1; // This form is usually 0x0 or a worldspace. coorX and
                     // coorY represent a cell in this worldspace.
  int32_t coorX;     // x-coordinate (cell coordinates) in worldSpace1.
  int32_t coorY;     // y-coordinate (cell coordinates) in worldSpace1
  RefID worldspace2; // This can be either a worldspace or an interior cell. If
                     // it's a worldspace, the player is located at the cell
                     // (coorX, coorY). posX/Y/Z is the player's position
                     // inside the cell.
  float posX;        // x-coordinate in worldSpace2
  float posY;        // y - coordinate in worldSpace2
  float posZ;        // u-coordinate in worldSpace2
  uint8_t unknown;   // vsval? It seems absent in 9th version
};

struct TES
{ // global data type=2
  struct Unknown0
  {
    RefID formID;
    uint16_t unknown;
  };

  uint32_t numUnknown1;
  std::vector<Unknown0> unknowns1;
  uint32_t numUnknown2;
  std::vector<RefID> unknowns2; //[numUnknown * 2]
  uint32_t numUnknown3;
  std::vector<RefID> unknowns3;
};

struct CreatedObjects
{ // global data type=4
  struct Enchantment
  {
    struct MagicEffect
    {
      struct EnchInfo
      {
        float magnitude;
        uint32_t duration;
        uint32_t area;
      };

      RefID effectID;
      EnchInfo info;
      float price; // Amount this enchantment adds to the base item's price.
    };

    RefID refID; // FormID of the enchantment. I've only seen created types, no
                 // default or array types.
    uint32_t
      timesUsed; // Seems to represent the number of times the enchantment is
                 // used? However, sometimes using the same enchantment nets
                 // two different forms. Could just be a bug in Skyrim.
    uint32_t numEffects;
    std::vector<MagicEffect> effects;
  };

  uint32_t numWeapon;
  std::vector<Enchantment>
    weaponEnchTable; // List of all created enchantments
                     // that are/were applied to weapons.
  uint32_t numArmor;
  std::vector<Enchantment>
    armourEnchTable; // List of all created enchantments that are/were applied
                     // to armour. Not sure which types of armour
                     // (Body/Gloves/Boots/Shield/etc) this encompasses.
  uint32_t numPotion;
  std::vector<Enchantment> potionTable; // List of all created potions.
  uint32_t numPoison;
  std::vector<Enchantment> poisonTable; // List of all created poisons.
};

struct Effects
{ // global data type=5
  struct Effect
  {
    float strength;  // Value from 0 to 1 (0 is no effect, 1 is full effect)
    float timestamp; // Time from effect beginning
    uint32_t
      unknown; // May be flag. Appears when you �dd a crossfade imagespace
               // modifier to the active list with imodcf command
    RefID effectID;
  };

  uint32_t numImgSpaceMod;
  std::vector<Effect> imageSpaceModifiers;
  float unknown1;
  float unknown2;
};

struct Weather
{ // global data type=6
  RefID climate;
  RefID weather;
  RefID prevWeather; // Only during weather transition. In other cases it
                     // equals zero.
  RefID unknownWeather1;
  RefID unknownWeather2;
  RefID regnWeather;
  float curTime;    // Current in-game time in hours
  float begTime;    // Time of current weather beginning
  float weatherPct; // A value from 0.0 to 1.0 describing how far in the
                    // current weather has transitioned
  uint32_t unknown1;
  uint32_t unknown2;
  uint32_t unknown3;
  uint32_t unknown4;
  uint32_t unknown5;
  uint32_t unknown6;
  float unknown7;
  uint32_t unknown8;
  uint8_t flags;
  void* unknown9;  // Unresearched format. Only present if flags has bit 0 set.
  void* unknown10; // Unresearched format. Only present if flags has bit 1 set.
};

struct Audio
{                // global data type=7
  RefID unknown; // Only the UIActivateFail sound descriptor has been observed
                 // here.
  uint32_t numTracks;
  std::vector<RefID>
    tracks;  // Seems to contain music tracks (MUST records) that were playing
             // at the time of saving, not including the background music.
  RefID bgm; // Background music at time of saving. Only MUST records have been
             // observed here.
};

struct SkyCells
{ // global data type=8
  struct Unknown0
  {
    RefID unknown1;
    RefID unknown2;
  };

  uint32_t numUnknown;
  std::vector<Unknown0> unknowns;
};

struct ProcessList
{ // global data type=100
  struct CrimeType
  {
    struct Crime
    {
      uint32_t witnessNum;
      uint32_t crimeType;

      enum CrimeTypes
      {
        Theft = 0,
        Pickpocketing = 1,
        Trespassing = 2,
        Assault = 3,
        Murder = 4,
        Unknown = 5,
        Lycanthropy = 6
      };

      uint8_t unknown1;
      uint32_t quantity; // The number of stolen items (e.g. if you've stolen
                         // Gold(7), it would be equals to 7).
      // Only for thefts
      uint32_t numSerial; // Assigned in accordance with nextNum
      uint8_t unknown2;
      uint32_t unknown3; // May be date of crime? Little byte is equal to day
      float elapsedTime; // Negative value measured from moment of crime
      RefID victimID;    // The killed, forced door, stolen item etc.
      RefID criminalID;
      RefID itemBaseID;  // Only for thefts
      RefID ownershipID; // Faction, outfit etc. Only for thefts
      uint32_t numWitnesses;
      std::vector<RefID> witnesses;
      uint32_t bounty;
      RefID crimeFactionID;
      uint8_t isCleared; // 0 - active crime, 1 - it was atoned
      uint16_t unknown4;
    };

    uint32_t numCrimes;
    std::vector<Crime> crimes;
  };

  float unknown1;
  float unknown2;
  float unknown3;
  uint32_t numNext; // This value is assigned to the next process
  CrimeType allCrimeTypes[numCrimesType]; // Crimes grouped according with
                                          // their type (see below)
};

struct Position
{
  float x;
  float y;
  float z;
  RefID cellID;
};

struct Combat
{ // global data type=101
  struct Unknown0_0
  {
    struct Unknown0_0_0
    {
      RefID unknown1;
      uint32_t unknown2;
      float unknown3;
      uint16_t unknown4;
      uint16_t unknown5;
      RefID target;
      Position unknown6;
      Position unknown7;
      Position unknown8;
      Position unknown9;
      Position unknown10;
      float unknown11;
      float unknown12;
      float unknown13;
      float unknown14;
      float unknown15;
      float unknown16;
    };

    struct Unknown0_0_1
    {
      RefID unknown1;
      float unknown2;
      float unknown3;
    };

    struct Unknown0_0_2
    {
      struct UnknownStruct
      {
        float unknown1;
        float unknown2;
      };

      struct Unknown0_0_2_0
      {
        Position unknown1;
        uint32_t unknown2;
        float unknown3;
      };

      struct Unknown0_0_2_1
      {
        RefID unknown1;
        RefID unknown2;
        uint8_t unknown3;
        uint8_t unknown4;
        uint8_t unknown5;
        uint8_t unknown6;
      };

      struct Unknown0_0_2_2
      {
        struct Unknown0_0_2_2_0
        {
          uint8_t unknown1;
          uint32_t numUnknown2;
          std::vector<uint8_t> unknowns2;
          RefID unknown3;
          uint32_t unknown4;
        };

        uint32_t unknown1;
        uint32_t unknown2;
        uint32_t numUnknown3;
        std::vector<Unknown0_0_2_2_0> unknowns3;
        RefID unknown4;
        float unknown5;
        float unknown6;
        float unknown7;
        float unknown8;
        float unknown9;
        RefID unknown10;
        float unknown11;
        RefID unknown12;
        uint16_t unknown13;
        uint8_t unknown14;
        uint8_t unknown15;
        float unknown16;
        float unknown17;
      };

      RefID unknown1;
      UnknownStruct unknown2;
      UnknownStruct unknown3;
      float unknown4;
      Position unknown5;
      float unknown6;
      uint32_t numUnknown7;
      std::vector<Unknown0_0_2_0> unknowns7;
      uint32_t numUnknown8;
      std::vector<Unknown0_0_2_1> unknowns8;
      uint8_t unknownFlag;
      Unknown0_0_2_2 unknown9; // only present if unknownFlag != 0
    };

    uint32_t numUnknown0;
    std::vector<Unknown0_0_0> unknowns0;
    uint32_t numUnknown1;
    std::vector<Unknown0_0_1> unknowns1;
    Unknown0_0_2::UnknownStruct unknown2;
    Unknown0_0_2::UnknownStruct unknown3;
    Unknown0_0_2::UnknownStruct unknown4;
    Unknown0_0_2::UnknownStruct unknown5;
    Unknown0_0_2::UnknownStruct unknowns6[11];
    uint32_t unknownFlag;
    Unknown0_0_2 unknown7; // only present if unknownFlag != 0
    Unknown0_0_2::UnknownStruct unknown8;
    float unknown9;
    float unknown10;
    float unknown11;
    float unknown12;
    Unknown0_0_2::UnknownStruct unknown13;
    uint8_t unknown14;
  };

  struct Unknown0
  {
    uint32_t unknown1;
    uint32_t serialNum; // Assigned in accordance with nextNum
    Unknown0_0 unknown2;
  };

  struct Unknown1
  {
    RefID unknown1;
    float unknown2;
    RefID unknown3;
    RefID unknown4;
    float unknown5;
    float unknown6;
    float unknown7;
    float x;
    float y;
    float z;
    float unknown8;
    float unknown9;
    float unknown10;
    float unknown11;
    float unknown12;
    float unknown13;
    float unknown14;
    float unknown15;
    RefID unknown16;
  };

  uint32_t nextNum; // This value is assigned to the next combat
  uint32_t numUnknown0;
  std::vector<Unknown0> unknowns0;
  uint32_t numUnknown1;
  std::vector<Unknown1> unknowns1;
  float unknown2;
  uint32_t unknown3;
  uint32_t numUnknown4;
  std::vector<RefID> unknowns4;
  float unknown5;
  Unknown0_0::Unknown0_0_2::UnknownStruct unknown6;
  Unknown0_0::Unknown0_0_2::UnknownStruct unknown7;
};

struct Interface
{ // global data type=102
  struct Unknown0
  {
    struct Unknown0_0
    {
      std::string unknown1;
      std::string unknown2;
      uint32_t unknown3;
      uint32_t unknown4;
      uint32_t unknown5;
      uint32_t unknown6;
    };

    uint32_t numUnknown1;
    std::vector<Unknown0_0> unknowns1;
    uint32_t numUnknown2;
    std::vector<std::string> unknowns2;
    uint32_t unknown3;
  };

  enum ShownMessagesType
  {
    HelpLockpickingShort = 0xEC,
    HelpSmithingShort = 0xEE,
    HelpCookingPots = 0xEF,
    HelpSmeltingShort = 0xF0,
    HelpTanningShort = 0xF1,
    HelpEnchantingShort = 0xF3,
    HelpGrindstoneShort = 0xF4,
    HelpArmorBenchShort = 0xF5,
    HelpAlchemyShort = 0xF6,
    HelpBarterShortPC = 0xF7,
    HelpLevelingShort = 0xF9,
    HelpWorldMapShortPC = 0xFA,
    HelpJournalShortPC = 0xFB,
    HelpJailTutorial = 0xFF,
    HelpFollowerCommandTutorial = 0x100,
    HelpFavoritesPCShort = 0x102,
    /// etc.
  };

  uint32_t numShownHelpMsg;
  std::vector<uint32_t> shownHelpMsg;
  uint8_t unknown1;
  uint32_t numLastUsedWeapons;
  std::vector<RefID> lastUsedWeapons;
  uint32_t numLastUsedSpells;
  std::vector<RefID> lastUsedSpells;
  uint32_t numLastUsedShouts;
  std::vector<RefID> lastUsedShouts;
  uint8_t unknown2;
  Unknown0 unknown3; // This value is only present in certain situations.
                     // Undetermined when.
};

struct ActorCauses
{ // global data type=103
  struct Unknown0
  {
    float x;
    float y;
    float z;
    uint32_t serialNum;
    RefID actorID;
  };

  uint32_t nextNum;
  uint32_t numUnknown;
  std::vector<Unknown0> unknowns;
};

struct DetectionManager
{ // global data type=105
  struct Unknown
  {
    RefID unknown1;
    uint32_t unknown2;
    uint32_t unknown3;
  };

  uint32_t numUnknown;
  std::vector<Unknown> unknowns;
};

struct LocationMetaData
{ // global data type=106
  struct Unknown
  {
    RefID unknown1;
    uint32_t unknown2;
  };

  uint32_t numUnknown;
  std::vector<Unknown> unknowns;
};

struct QuestStaticData
{ // global data type=107
  struct Unknown0
  {
    struct Unknown0_1
    {
      uint32_t unknown0;
      uint32_t unknown1;
    };
    RefID unknown0;
    uint32_t numUnknown1;
    std::vector<Unknown0_1> unknowns1;
  };
  struct QuestRunData_3
  {
    struct QuestRunData_3_item
    {
      uint32_t type;
      std::shared_ptr<void>
        unknown; // depends on previous type - 1,2,4 -> refID; 3 -> uint32
    };
    uint32_t unknown0;
    float unknown1;
    uint32_t numQuestDataItems;
    std::vector<QuestRunData_3_item> questRunData_items;
  };
  uint32_t numUnknown0;
  std::vector<QuestRunData_3> unknowns0;
  uint32_t numUnknown1;
  std::vector<QuestRunData_3> unknowns1;
  uint32_t numUnknown2;
  std::vector<RefID> unknowns2;
  uint32_t numUnknown3;
  std::vector<RefID> unknowns3;
  uint32_t numUnknown4;
  std::vector<RefID> unknowns4;
  uint32_t numUnknown5;
  std::vector<Unknown0> unknowns5;
  uint8_t unknown6; // always seems to be 1
};

struct StoryTeller
{               // global data type=108
  uint8_t flag; // 0 or 1
};

struct MagicFavorites
{ // global data type=109
  uint32_t numFavoritedMagics;
  std::vector<RefID> favoritedMagics; // Spells, shouts, abilities etc.
  uint32_t numMagicHotKeys;
  std::vector<RefID>
    magicHotKeys; // Hotkey corresponds to the position of magic in this array
};

struct PlayerControls
{ // global data type=110
  uint8_t unknown1;
  uint8_t unknown2;
  uint8_t unknown3;
  uint16_t unknown4;
  uint8_t unknown5;
};

struct StoryEventManager
{ /// global data type=111 TODO
  uint32_t unknown0;
  uint32_t count;
  // void* unknown1; //Unknown format. Possibly the same as unk0 and unk1 in
  // Quest Static Data
  std::vector<uint8_t> data;
};

struct IngredientShared
{ // global data type=112
  struct IngredientsCombined
  {
    RefID ingredient0;
    RefID ingredient1;
  };

  uint32_t numIngredientsCombined;
  std::vector<IngredientsCombined>
    ingredientsCombined; // Pairs of failed ingredient combinations in alchemy.
};

struct MenuControls
{ // global data type=113
  uint8_t unknown1;
  uint8_t unknown2;
};

struct MenuTopicManager
{ // global data type=114
  RefID unknown1;
  RefID unknown2;
};

struct TempEffects
{ /// TODO REFACTORING THIS global data type=1000
  struct Unknown0
  {
    uint8_t flag;
    RefID unknown1;
    uint32_t unknown2; // Only present if flag is non zero
    RefID unknown3;
    RefID unknown4;
    float unknown5[3];
    float unknown6[3];
    float unknown7;
    float unknown8;
    float unknown9;
    float unknown10[4];
    uint8_t unknown11;
    uint8_t unknown12;
    uint8_t unknown13;
    uint8_t unknown14;
    float unknown15;
    uint8_t unknown16;
    float unknown17;
    float unknown18;
    float unknown19;
    float unknown20;
    float unknown21;
    uint8_t unknown22;
    uint8_t unknown23;
    uint32_t unknown24;
  };

  struct Unknown1_0
  {
    struct Unknown1_def
    {
      float unknown1;
      float unknown2;
      uint8_t unknown3;
      uint32_t unknown4;
    };

    Unknown1_def unknown1;
    uint32_t unknown2[4];
    uint32_t unknown3[3];
    uint8_t unknown4[12];
    std::string unknown5;
    RefID unknown6;
    RefID unknown7;
    uint32_t unknown8;
  };

  struct Unknown1_6
  {
    Unknown1_0::Unknown1_def unknown1;
    RefID unknown2;
    uint32_t unknown3;
    uint32_t unknown4;
    uint8_t unknown5;
  };

  struct Unknown1_8
  {
    Unknown1_0::Unknown1_def unknown1;
    uint8_t unknown2;
    RefID unknown3;
    uint8_t flag;
    RefID unknown4[4]; // only present if flag is non-zero
  };

  struct Unknown1_9_0_0
  {
    struct Unknown0
    { //{wstring, uint32, wstring, uint32, uint32}[count0]
      std::string u0;
      uint32_t u1;
      std::string u2;
      uint32_t u3;
      uint32_t u4;
    };

    struct Unknown1
    { //{wstring, uint32}[count1]
      std::string u0;
      uint32_t u1;
    };

    struct Unknown2
    { //{wstring, float, {wstring, uint8, uint32}[2]}[count2]
      struct u2_t
      {
        std::string u0;
        uint8_t u1;
        uint32_t u2;
      };

      std::string u0;
      float u1;
      u2_t u2[2];
    };

    struct Unknown3
    { //{wstring, uint8}[count3]
      std::string u0;
      uint8_t u1;
    };

    struct Unknown4
    { //{ wstring, wstring, uint32, uint32, uint16, uin16, uint16, uint8, uint8
      //}[count4]
      std::string u0;
      std::string u1;
      uint32_t u2;
      uint32_t u3;
      uint16_t u4;
      uint16_t u5;
      uint16_t u6;
      uint8_t u7;
      uint8_t u8;
    };

    struct Unknown5
    { //{wstring, uint32}[count5]
      std::string u0;
      uint32_t u1;
    };

    struct Unknown6
    { //{wstring, uint32}[count6]
      std::string u0;
      uint32_t u1;
    };

    struct Unknown7
    { //{wstring, uint32 [count1], uint32 [count2], uint16[count1],
      // uint16[count2]}[count7]
      std::string u0;
      uint32_t* u1; //[count1]
      uint32_t* u2; //[count2]
      uint16_t* u3; //[count1]
      uint16_t* u4; //[count2]
    };

    struct Unknown8
    { //{wstring, uint8}[count8]
      std::string u0;
      uint8_t u1;
    };

    struct Unknown9
    { //{wstring, uint32[4], uint32[4], uint32[4], uint32, uint32}[count9]
      std::string u0;
      uint32_t u1[4];
      uint32_t u2[4];
      uint32_t u3[4];
      uint32_t u4;
      uint32_t u5;
    };

    struct Unknown10
    { //{wstring, 3 x uint32[4], 3 x uint32[4], 3 x uint32[4], uint32,
      // uint8}[count10] ????
      std::string u0;
      uint32_t u1[4];
      uint32_t u2[4];
      uint32_t u3[4];
      uint32_t u4[4];
      uint32_t u5[4];
      uint32_t u6[4];
      uint32_t u7[4];
      uint32_t u8[4];
      uint32_t u9[4];
      uint32_t u10;
      uint8_t u11;
    };

    struct Unknown11
    { //{wstring, wstring}[count11]
      std::string u0;
      std::string u1;
    };

    struct Unknown12
    { //{uint16, uint32, uint32, uint32, uint8, uint32}[count12]
      uint16_t u0;
      uint32_t u1;
      uint32_t u2;
      uint32_t u3;
      uint8_t u4;
      uint32_t u5;
    };

    std::string unknownStr0;
    uint32_t numUnknows0;
    std::vector<Unknown0> unknowns0;
    uint32_t numUnknows1;
    std::vector<Unknown1> unknowns1;
    uint32_t numUnknows2;
    std::vector<Unknown2> unknowns2;
    std::string unknownStr1;
    uint32_t numUnknows3;
    std::vector<Unknown3> unknowns3;
    uint32_t numUnknows4;
    std::vector<Unknown4> unknowns4;
    uint32_t numUnknows5;
    std::vector<Unknown5> unknowns5;
    uint32_t numUnknows6;
    std::vector<Unknown6> unknown6;
    uint32_t numUnknows7;
    std::vector<Unknown7> unknowns7;
    uint32_t numUnknows8;
    std::vector<Unknown8> unknowns8;
    uint32_t numUnknows9;
    std::vector<Unknown9> unknowns9;
    uint32_t numUnknows10;
    std::vector<Unknown10> unknowns10;
    uint32_t numUnknows11;
    std::vector<Unknown11> unknowns11;
    uint32_t numUnknows12;
    std::vector<Unknown12> unknowns12;
  };

  struct Unknown1_9_0
  {
    uint32_t length; // Length of the next data
    uint8_t unknown;
    uint32_t numUnknow0;
    std::vector<Unknown1_9_0_0> unknowns0;
  };

  struct TempEffects_Unknown1_9
  {
    Unknown1_8 unknown1;
    RefID unknown2;
    Unknown1_9_0 unknown3;
  };

  struct TempEffects_Unknown1_10
  {
    Unknown1_8 unknown1;
    float unknown2;
    float unknown3;
    float unknown4;
    float unknown5;
    uint32_t unknown6;
    RefID unknown7;
    RefID unknown8;
    uint32_t unknown9;
  };

  struct TempEffects_Unknown1_11
  {
    Unknown1_8 unknown1;
    RefID unknown2;
    uint8_t unknown3;
    uint32_t unknown4[3];
    Unknown1_9_0 unknown5;
  };

  struct Unknown1
  {
    uint32_t unknown1;              // Probably type of temp effect
    std::shared_ptr<void> unknown2; // type seems to depend on previous
                                    // variable
  };

  uint32_t numUnknow0;
  std::vector<Unknown0> unknowns0;
  uint32_t unknown;
  uint32_t numUnknow1;
  std::vector<Unknown1> unknowns1;
  uint32_t numUnknow2;
  std::vector<Unknown1> unknowns2;
};

struct AnimObjects
{ // global data type=1002
  struct AnimObject
  {
    RefID achr;      // RefID pointing to an actor reference.
    RefID anim;      // RefID pointing to an animation form.
    uint8_t unknown; // Unknown but only 0 and 1 have been observed.
  };

  uint32_t numObjects;
  std::vector<AnimObject>
    objects; // Array with currently active actor reference + animation combo?
  // Haven't yet determined when these are saved.
};

struct Timer
{ // global data type=1003
  uint32_t unknown1;
  uint32_t unknown2;
};
}
