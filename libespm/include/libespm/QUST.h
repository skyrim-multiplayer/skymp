#pragma once
#include "RecordHeader.h"
#include "ScriptData.h"

#pragma pack(push, 1)

namespace espm {

class QUST final : public RecordHeader
{
public:
  static constexpr auto kType = "QUST";

  struct QuestData
  {
    enum MainFlags : uint8_t
    {
      StartGameEnabled = 0x01,
      Unused = 0x02,
      WildernessEncounter = 0x04,
      AllowRepeatedStages = 0x08,
      Unknown = 0x10
    };

    enum PrimaryFlags : uint8_t
    {
      RunOnce = 0x01,
      ExcludeFromDialogueExport = 0x02,
      WarnOnAliasFillFailure = 0x04,
      Unused = 0x08,
      Unknown = 0x10
    };

    enum QuestType : uint32_t
    {
      None,
      MainQuest,
      MagesGuild,
      ThievesGuild,
      DarkBrotherhood,
      ComponionQuests,
      Miscellaneous,
      DaedricQuests,
      SideQuests,
      CivilWar,
      DLC01Vampire,
      DLC02Dragonborn
    };

    MainFlags mainFlags = MainFlags::Unknown;
    PrimaryFlags primaryFlags = PrimaryFlags::Unknown;
    uint8_t priority = 0;
    uint8_t unknown = 0;
    uint32_t unknown2 = 0;
    QuestType questType = QuestType::None;
  };
  static_assert(sizeof(QuestData) == 12);

  struct QuestStage
  {
    struct JournalIndex
    {
      struct QuestLogEntry
      {
        enum LogFlags : Byte
        {
          CompleteQuest = 0x01,
          FailQuest = 0x02
        };

        LogFlags logFlags;
        // skip vector<CTDA>
        const char* journalEntry;
        uint32_t unknown;
        // skip SCHR
      };

      enum IndexFlags : uint8_t
      {
        StartUpStage = 0x02,
        ShutDownStage = 0x04,
        KeepInstanceDataFromHereOn = 0x08
      };

      int16_t index;
      IndexFlags flags;
      uint8_t unknown;
      std::vector<QuestLogEntry> logEntries;
    };

    // Multiple sections can appear in a single record, and 1 or more log entries can appear within each INDX field. 
    std::vector<JournalIndex> journalIndexes;
  };

  struct QuestObjective
  {
    struct QuestTarget
    {
      int32_t targetAlias;
      int32_t flags;
      // skip vector<CTDA>
    };

    int16_t index;
    int32_t flags;
    const char* nodeName;
    std::vector<QuestTarget> questTarets;
  };

  struct Alias
  {
    struct Container
    {
      uint32_t ItemID;
      uint32_t ItemCount;
    };
    static_assert(sizeof(Container) == 8);

    enum AliasFlags : uint32_t
    {
      ReservesLocationOrReference = 0x01,
      Optional = 0x02,
      QuestObject = 0x04,
      AllowReuseInQuest = 0x08,
      AllowDead = 0x10,
      InLoadedArea = 0x20,
      Essential = 0x40,
      AllowDisabled = 0x80,
      StoresText = 0x100,
      AllowReserved = 0x200,
      Protected = 0x400,
      NoFillType = 0x800,
      AllowDestroyed = 0x1000,
      Closest = 0x2000,
      UsesStoredText = 0x4000,
      InitiallyDisabled = 0x8000,
      AllowCleared = 0x10000,
      ClearsNameWhenRemoved = 0x20000
    };

    enum Level : uint32_t
    {
      Easy,
      Medium,
      Hard,
      VeryHard,
      None
    };

    uint32_t aliasID;
    uint32_t locationID;
    const char* aliasName;
    AliasFlags flags;
    uint32_t forcedIntoAlias;
    uint32_t aliasCreatedObject;
    uint32_t createAt;
    Level createLevel;
    uint32_t externalAliasReference;
    uint32_t externalAlias;
    uint32_t referenceAliasLocation;
    uint32_t keyword;
    uint32_t locationAliasReference;
    uint32_t aliasReferenceType;
    const char* fromEvent;
    const char* eventData;
    uint32_t aliasForcedLocation;
    uint32_t aliasForcedReference;
    uint32_t nearAlias;
    uint32_t nearType;
    uint32_t aliasUniqueActor;
    // skip vector<CTDA>
    uint32_t keywordsCount;
    std::vector<uint32_t> keywords;
    uint32_t containersCount;
    std::vector<Container> containers;
    uint32_t spectatorOverride;
    uint32_t observeDeadBodyOverride;
    uint32_t guardWarnOverride;
    uint32_t combatOverride;
    uint32_t displayName;
    std::vector<uint32_t> aliasSpells;
    std::vector<uint32_t> aliasFactions;
    std::vector<uint32_t> aliasPackageData;
    uint32_t voiceType;
    // ALED (EOF Marker) - empty - Marks the end of any given alias entry.
  };

  struct Data
  {
    const char* editorId = "";
    ScriptData scriptData;
    QuestData questData;
    const char* eventName = "";
    uint32_t displayGlobals = 0;
    const char* objectWindowFilter;
    // skip vector<CTDA>
    std::vector<QuestStage> questStages;
    std::vector<QuestObjective> questObjectives;
    int32_t nextAliasId = 0;
    std::vector<Alias> aliases;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(QUST) == sizeof(RecordHeader));

}

#pragma pack(pop)
