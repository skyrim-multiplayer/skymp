#pragma once
#include <array>
#include <cstdint>
#include <cstring> // memcmp
#include <functional>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#pragma pack(push, 1)

namespace espm {
class CompressedFieldsCache
{
public:
  CompressedFieldsCache();
  ~CompressedFieldsCache();

  struct Impl;
  Impl* const pImpl;
};

union CellOrGridPos
{
  uint32_t cellId = 0;
  struct
  {
    int16_t y;
    int16_t x;
  } pos;
};

class GroupHeader;
class RecordHeader;
class ScriptData;
struct Effects;

using GroupStack = std::vector<espm::GroupHeader*>;

class Browser
{
public:
  Browser(const void* fileContent, size_t length);
  ~Browser();

  RecordHeader* LookupById(uint32_t formId) const noexcept;

  std::pair<espm::RecordHeader**, size_t> FindNavMeshes(
    uint32_t worldSpaceId, CellOrGridPos cellOrGridPos) const noexcept;

  const std::vector<espm::RecordHeader*>& GetRecordsByType(
    const char* type) const;

  const std::vector<espm::RecordHeader*>& GetRecordsAtPos(uint32_t cellOrWorld,
                                                          int16_t cellX,
                                                          int16_t cellY) const;

  const GroupStack* GetParentGroupsOptional(const RecordHeader* rec) const;
  const GroupStack& GetParentGroupsEnsured(const RecordHeader* rec) const;

  const std::vector<void*>* GetSubsOptional(const GroupHeader* group) const;
  const std::vector<void*>& GetSubsEnsured(const GroupHeader* group) const;

private:
  struct Impl;
  Impl* const pImpl;

  bool ReadAny(const GroupStack* parentGrStack);

  Browser(const Browser&) = delete;
  void operator=(const Browser&) = delete;
};

enum class GroupType : uint32_t
{
  TOP,
  WORLD_CHILDREN,
  INTERIOR_CELL_BLOCK,
  INTERIOR_CELL_SUBBLOCK,
  EXTERIOR_CELL_BLOCK,
  EXTERIOR_CELL_SUBBLOCK,
  CELL_CHILDREN,
  TOPIC_CHILDREN,
  CELL_PERSISTENT_CHILDREN,
  CELL_TEMPORARY_CHILDREN,
  CELL_VISIBLE_DISTANT_CHILDREN
};
static_assert((int)GroupType::CELL_VISIBLE_DISTANT_CHILDREN == 10);

inline std::string ToString(GroupType groupType)
{
  switch (groupType) {
    case GroupType::TOP:
      return "TOP";
    case GroupType::WORLD_CHILDREN:
      return "WORLD_CHILDREN";
    case GroupType::INTERIOR_CELL_BLOCK:
      return "INTERIOR_CELL_BLOCK";
    case GroupType::INTERIOR_CELL_SUBBLOCK:
      return "INTERIOR_CELL_SUBBLOCK";
    case GroupType::EXTERIOR_CELL_BLOCK:
      return "EXTERIOR_CELL_BLOCK";
    case GroupType::EXTERIOR_CELL_SUBBLOCK:
      return "EXTERIOR_CELL_SUBBLOCK";
    case GroupType::CELL_CHILDREN:
      return "CELL_CHILDREN";
    case GroupType::TOPIC_CHILDREN:
      return "TOPIC_CHILDREN";
    case GroupType::CELL_PERSISTENT_CHILDREN:
      return "CELL_PERSISTENT_CHILDREN";
    case GroupType::CELL_TEMPORARY_CHILDREN:
      return "CELL_TEMPORARY_CHILDREN";
    case GroupType::CELL_VISIBLE_DISTANT_CHILDREN:
      return "CELL_VISIBLE_DISTANT_CHILDREN";
    default:
      throw std::runtime_error("unhandled case in ToString");
  }
}

class GroupHeader
{
  friend class Browser;

public:
  bool GetXY(int16_t& outX, int16_t& outY) const noexcept;
  const char* GetRecordsType() const noexcept;
  bool GetBlockNumber(int32_t& outBlockNum) const noexcept;
  bool GetSubBlockNumber(int32_t& outSubBlockNum) const noexcept;
  bool GetParentWRLD(uint32_t& outId) const noexcept;
  bool GetParentCELL(uint32_t& outId) const noexcept;
  bool GetParentDIAL(uint32_t& outId) const noexcept;

  uint32_t GetGroupLabelAsUint() const noexcept;
  GroupType GetGroupType() const noexcept;

private:
  char label[4];
  GroupType grType;

  uint8_t day;
  uint8_t months;
  uint16_t unknown;
  uint16_t version;
  uint16_t unknown2;

  GroupHeader() = delete;
  GroupHeader(const GroupHeader&) = delete;
  void operator=(const GroupHeader&) = delete;
};
static_assert(sizeof(GroupType) == 4);
static_assert(sizeof(GroupHeader) == 16);

using IdMapping = std::array<uint8_t, 256>;
uint32_t GetMappedId(uint32_t id, const IdMapping& mapping) noexcept;

class Type
{
public:
  // Type object doesn't own this pointer
  Type(const char* type_)
    : type(type_){};

  bool operator==(const char* rhs) const noexcept
  {
    return !memcmp(type, rhs, 4);
  }

  bool operator!=(const char* rhs) const noexcept { return !(*this == rhs); }

  std::string ToString() const noexcept { return std::string(type, 4); }

  uint32_t ToUint32() const noexcept
  {
    return *reinterpret_cast<const uint32_t*>(type);
  }

private:
  const char* type;
};

class RecordHeaderAccess;

class RecordHeader
{
  friend class espm::Browser;
  friend class RecordHeaderAccess;

public:
  uint32_t GetId() const noexcept;
  const char* GetEditorId(
    espm::CompressedFieldsCache& compressedFieldsCache) const noexcept;
  void GetScriptData(
    ScriptData* out,
    espm::CompressedFieldsCache& compressedFieldsCache) const noexcept;
  std::vector<uint32_t> GetKeywordIds(
    espm::CompressedFieldsCache& compressedFieldsCache) const noexcept;

  Type GetType() const noexcept;

  // Please use for tests only
  // Do not rely on Skyrim record flags format
  uint32_t GetFlags() const noexcept;

private:
  uint32_t flags;
  uint32_t id;
  uint32_t revision;
  uint16_t version;
  uint16_t unk;

  uint32_t GetFieldsSizeSum() const noexcept;

  RecordHeader() = delete;
  RecordHeader(const RecordHeader&) = delete;
  void operator=(const RecordHeader&) = delete;
};
static_assert(sizeof(RecordHeader) == 16);

// Helpers/utilities

inline bool IsItem(Type t) noexcept
{
  return t == "AMMO" || t == "ARMO" || t == "BOOK" || t == "INGR" ||
    t == "ALCH" || t == "SCRL" || t == "SLGM" || t == "WEAP" || t == "MISC";
}
}

namespace espm {
template <class RecordT>
const RecordT* Convert(const RecordHeader* source)
{
  if (source && source->GetType() == RecordT::kType) {
    return (const RecordT*)source;
  }
  return nullptr;
}
}

namespace espm {
class TES4 : public RecordHeader
{
public:
  static constexpr auto kType = "TES4";

  // Header
  struct Header
  {
    float version = 0;
    int32_t numRecords = 0;
    uint32_t nextObjectId = 0;
  };
  static_assert(sizeof(Header) == 12);

  struct Data
  {
    const Header* header = nullptr;
    const char* author = "";
    const char* description = "";
    std::vector<const char*> masters;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
  ;
};
static_assert(sizeof(TES4) == sizeof(RecordHeader));

// reinterpret_cast<REFR *> must be safe for ACHR records
class REFR : public RecordHeader
{
public:
  static constexpr auto kType = "REFR";

  struct LocationalData
  {
    float pos[3];
    float rotRadians[3];
  };

  struct DoorTeleport
  {
    uint32_t destinationDoor = 0;
    float pos[3];
    float rotRadians[3];
  };

  struct Data
  {
    uint32_t baseId = 0;
    float scale = 1;
    const LocationalData* loc = nullptr;
    const DoorTeleport* teleport = nullptr;
    const float* boundsDiv2 = nullptr;
    uint32_t count = 0;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
  ;
};
static_assert(sizeof(REFR) == sizeof(RecordHeader));

class ACHR : public RecordHeader
{
public:
  static constexpr auto kType = "ACHR";
};
static_assert(sizeof(ACHR) == sizeof(RecordHeader));

class CONT : public RecordHeader
{
public:
  static constexpr auto kType = "CONT";

  struct ContainerObject
  {
    uint32_t formId = 0;
    uint32_t count = 0;
  };

  struct Data
  {
    const char* editorId = "";
    const char* fullName = "";
    std::vector<ContainerObject> objects;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
  ;
};
static_assert(sizeof(CONT) == sizeof(RecordHeader));

struct ObjectBounds
{
  int16_t pos1[3] = { 0, 0, 0 };
  int16_t pos2[3] = { 0, 0, 0 };
};
static_assert(sizeof(ObjectBounds) == 12);

class TREE : public RecordHeader
{
public:
  static constexpr auto kType = "TREE";

  struct Data
  {
    const char* editorId = "";
    const char* fullName = "";
    const ObjectBounds* bounds = nullptr;
    uint32_t resultItem = 0;
    uint32_t useSound = 0;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
  ;
};
static_assert(sizeof(TREE) == sizeof(RecordHeader));

class FLOR : public RecordHeader
{
public:
  static constexpr auto kType = "FLOR";

  using Data = TREE::Data;

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
  ;
};
static_assert(sizeof(TREE) == sizeof(RecordHeader));

class DOOR : public RecordHeader
{
public:
  static constexpr auto kType = "DOOR";
};
static_assert(sizeof(DOOR) == sizeof(RecordHeader));

class LVLI : public RecordHeader
{
public:
  static constexpr auto kType = "LVLI";

  enum LeveledItemFlags
  {
    AllLevels = 0x01, //(sets it to calculate for all entries < player level,
                      // choosing randomly from all the entries under)
    Each = 0x02, // (sets it to repeat a check every time the list is called
                 // (if it's called multiple times), otherwise it will use the
                 // same result for all counts.)
    UseAll = 0x04, // (use all entries when the list is called)
    SpecialLoot = 0x08,
  };

  struct Entry
  {
    char type[4] = { 'L', 'V', 'L', 'O' };
    uint16_t dataSize = 0;
    uint32_t level = 0;
    uint32_t formId = 0;
    uint32_t count = 0;
  };

  struct Data
  {
    const char* editorId = "";
    uint8_t chanceNone = 0;
    uint8_t leveledItemFlags = 0;
    uint32_t chanceNoneGlobalId = 0;
    uint8_t numEntries = 0;
    Entry* entries = nullptr;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
  ;
};
static_assert(sizeof(LVLI) == sizeof(RecordHeader));
static_assert(sizeof(LVLI::Entry) == 18);

class NAVM : public RecordHeader
{
public:
  static constexpr auto kType = "NVNM";

  class Vertices
  {
  public:
    Vertices(const void* nvnmField);

    const std::array<float, 3>* begin() const noexcept;
    const std::array<float, 3>* end() const noexcept;

  private:
    const int32_t* numVerticesPtr = nullptr;
    const std::array<float, 3>* beginPtr = nullptr;
    const void* nvnmField;
  };

  struct Data
  {
    std::unique_ptr<Vertices> vertices;
    uint32_t worldSpaceId = 0;
    CellOrGridPos cellOrGridPos = { 0 };
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(REFR) == sizeof(RecordHeader));

class FLST : public RecordHeader
{
public:
  static constexpr auto kType = "FLST";

  struct Data
  {
    std::vector<uint32_t> formIds;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
  ;
};
static_assert(sizeof(FLST) == sizeof(RecordHeader));

enum class PropertyType
{
  Invalid = 0,
  Object = 1,
  String = 2,
  Int = 3,
  Float = 4,
  Bool = 5,
  ObjectArray = 11,
  StringArray = 12,
  IntArray = 13,
  FloatArray = 14,
  BoolArray = 15
};

struct Property
{
  enum : uint8_t
  {
    StatusEdited = 1,
    StatusRemoved = 3
  };

  static Property Object(std::string propertyName, uint32_t formId)
  {
    Property res{ propertyName, PropertyType::Object };
    res.value.formId = formId;
    return res;
  }

  static Property Int(std::string propertyName, int32_t integer)
  {
    Property res{ propertyName, PropertyType::Int };
    res.value.integer = integer;
    return res;
  }

  static Property Bool(std::string propertyName, bool boolean)
  {
    Property res{ propertyName, PropertyType::Bool };
    res.value.boolean = boolean ? 1 : 0;
    return res;
  }

  static Property Float(std::string propertyName, float floatingPoint)
  {
    Property res{ propertyName, PropertyType::Float };
    res.value.floatingPoint = floatingPoint;
    return res;
  }

  friend std::ostream& operator<<(std::ostream& os, const Property& prop)
  {
    os << "[" << prop.propertyName;
    switch (prop.propertyType) {
      case PropertyType::Bool:
        os << "=" << (prop.value.boolean ? "true" : "false");
        break;
      case PropertyType::Int:
        os << "=" << prop.value.integer;
        break;
      case PropertyType::Float:
        os << "=" << prop.value.floatingPoint;
        break;
      case PropertyType::Object:
        os << "=FormID_" << std::hex << prop.value.formId << std::dec;
        break;
      default:
        os << "=...";
        break;
    }
    os << ", status=" << static_cast<int>(prop.status) << "]";
    return os;
  }

  std::string propertyName;
  PropertyType propertyType = PropertyType::Invalid;

  union Value
  {
    uint32_t formId;
    int32_t integer;
    int8_t boolean;
    float floatingPoint;

    struct Str
    {
      const char* data;
      size_t length;
    } str = { 0, 0 };
  } value;

  std::vector<Value> array;

  uint8_t status = StatusEdited;

  auto ToTuple() const
  {
    return std::make_tuple(propertyName, propertyType, value.str.data,
                           value.str.length, status);
  }

  friend bool operator==(const Property& lhs, const Property& rhs)
  {
    return lhs.ToTuple() == rhs.ToTuple();
  }

  friend bool operator!=(const Property& lhs, const Property& rhs)
  {
    return !(lhs == rhs);
  }

  friend bool operator<(const Property& lhs, const Property& rhs)
  {
    return lhs.ToTuple() < rhs.ToTuple();
  }
};

using IterateFieldsCallback =
  std::function<void(const char* type, uint32_t size, const char* data)>;

void IterateFields_(const espm::RecordHeader* rec,
                    const espm::IterateFieldsCallback& f,
                    espm::CompressedFieldsCache& compressedFieldsCache);

struct Script
{
  std::string scriptName;
  uint8_t status = 0;
  std::set<Property> properties;
};

struct ScriptData
{
  int16_t version = 0;   // [2..5]
  int16_t objFormat = 0; // [1..2]
  std::vector<Script> scripts;
};

class ACTI : public RecordHeader
{
public:
  static constexpr auto kType = "ACTI";

  struct Data
  {
    ScriptData scriptData;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(ACTI) == sizeof(RecordHeader));

class COBJ : public RecordHeader
{
public:
  static constexpr auto kType = "COBJ";

  struct InputObject
  {
    uint32_t formId = 0;
    uint32_t count = 0;
  };
  static_assert(sizeof(InputObject) == 8);

  struct Data
  {
    std::vector<InputObject> inputObjects;
    uint32_t outputObjectFormId = 0;
    uint32_t benchKeywordId = 0;
    uint32_t outputCount = 0;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(COBJ) == sizeof(RecordHeader));

class OTFT : public RecordHeader
{
public:
  static constexpr auto kType = "OTFT";

  struct Data
  {
    const uint32_t* formIds = nullptr;
    uint32_t count = 0;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(OTFT) == sizeof(RecordHeader));

class NPC_ : public RecordHeader
{
public:
  static constexpr auto kType = "NPC_";

  struct Faction
  {
    uint32_t formId = 0;
    int8_t rank = 0;
  };

  struct Data
  {
    uint32_t defaultOutfitId = 0;
    uint32_t sleepOutfitId = 0;

    std::set<uint32_t> spells = {};

    std::vector<CONT::ContainerObject> objects;
    std::vector<Faction> factions;

    bool isEssential = false;
    bool isProtected = false;

    uint32_t race = 0;
    uint16_t healthOffset = 0;
    uint16_t magickaOffset = 0;
    uint16_t staminaOffset = 0;
    ObjectBounds objectBounds = {};
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(NPC_) == sizeof(RecordHeader));

class WEAP : public RecordHeader
{
public:
  static constexpr auto kType = "WEAP";

  struct WeapData
  {
    int32_t value = 0;
    float weight = 0.f;
    int16_t damage = 0;
  };
  static_assert(sizeof(WeapData) == 10);

  enum class AnimType : uint8_t
  {
    Other = 0,
    OneHandSword = 1,
    OneHandDagger = 2,
    OneHandAxe = 3,
    OneHandMace = 4,
    TwoHandSword = 5,
    TwoHandAxe = 6,
    Bow = 7,
    Staff = 8,
    Crossbow = 9
  };
  static_assert(sizeof(AnimType) == 1);

  struct DNAM
  {
    AnimType animType = AnimType::Other;
    uint8_t unknown01 = 0;
    uint16_t unknown02 = 0;
    float speed = 0.f;
    float reach = 0.f;
    // 0C: flags, etc
  };
  static_assert(sizeof(DNAM) == 0x0c);

  struct Data
  {
    const WeapData* weapData = nullptr;
    const DNAM* weapDNAM = nullptr;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(WEAP) == sizeof(RecordHeader));

class ARMO : public RecordHeader
{
public:
  static constexpr auto kType = "ARMO";

  struct Data
  {
    uint32_t baseRatingX100 = 0;
    uint32_t baseValue = 0;
    float weight = 0;
    uint32_t enchantmentFormId = 0;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const;
};
static_assert(sizeof(WEAP) == sizeof(RecordHeader));

class RACE : public RecordHeader
{
public:
  static constexpr auto kType = "RACE";

  struct Data
  {
    float startingHealth = 0.f;
    float startingMagicka = 0.f;
    float startingStamina = 0.f;
    float healRegen = 0.f;
    float magickaRegen = 0.f;
    float staminaRegen = 0.f;
    float unarmedDamage = 0.f;
    float unarmedReach = 0.f;

    std::set<uint32_t> spells = {};
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(RACE) == sizeof(RecordHeader));

// game settings
class GMST : public RecordHeader
{
public:
  static constexpr auto kType = "GMST";

  static constexpr uint32_t kFCombatDistance = 0x00055640;
  static constexpr uint32_t kFMaxArmorRating = 0x00037DEB;
  static constexpr uint32_t kFArmorScalingFactor = 0x00021A72;

  struct Data
  {
    float value = 0.f;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(GMST) == sizeof(RecordHeader));

struct Effects
{
public:
  Effects(){};
  Effects(const RecordHeader* parent);
  const espm::RecordHeader* parent = nullptr;

  struct Effect
  {
    uint32_t effectId = 0; // Corresponding MGEF record id
    float magnitude = 0.f;
    uint32_t areaOfEffect = 0;
    uint32_t duration = 0;
  };

  struct Data
  {
    std::vector<Effect> effects;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

class ENCH : public RecordHeader
{
public:
  static constexpr auto kType = "ENCH";

  struct Data
  {
    std::vector<espm::Effects::Effect> effects;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(ENCH) == sizeof(RecordHeader));

enum class ActorValue : int32_t
{
  None = -1,
  Aggression,
  Confidence,
  Energy,
  Morality,
  Mood,
  Assistance,
  OneHanded,
  TwoHanded,
  Marksman,
  Block,
  Smithing,
  HeavyArmor,
  LightArmor,
  Pickpocket,
  Lockpicking,
  Sneak,
  Alchemy,
  Speechcraft,
  Alteration,
  Conjuration,
  Destruction,
  Illusion,
  Restoration,
  Enchanting,
  Health,
  Magicka,
  Stamina,
  HealRate,
  MagickaRate,
  StaminaRate,
  SpeedMult,
  InventoryWeight,
  CarryWeight,
  CritChance,
  MeleeDamage,
  UnarmedDamage,
  Mass,
  VoicePoints,
  VoiceRate,
  DamageResist,
  PoisonResist,
  FireResist,
  ElectricResist,
  FrostResist,
  MagicResist,
  DiseaseResist,
  PerceptionCondition,
  EnduranceCondition,
  LeftAttackCondition,
  RightAttackCondition,
  LeftMobilityCondition,
  RightMobilityCondition,
  BrainCondition,
  Paralysis,
  Invisibility,
  NightEye,
  DetectLifeRange,
  WaterBreathing,
  WaterWalking,
  IgnoreCrippledLimbs,
  Fame,
  Infamy,
  JumpingBonus,
  WardPower,
  RightItemCharge_or_EquippedItemCharge,
  ArmorPerks,
  ShieldPerks,
  WardDeflection,
  Variable01,
  Variable02,
  Variable03,
  Variable04,
  Variable05,
  Variable06,
  Variable07,
  Variable08,
  Variable09,
  Variable10,
  BowSpeedBonus,
  FavorActive,
  FavorsPerDay,
  FavorsPerDayTimer,
  LeftItemCharge_or_EquippedStaffCharge,
  AbsorbChance,
  Blindness,
  WeaponSpeedMult,
  ShoutRecoveryMult,
  BowStaggerBonus,
  Telekinesis,
  FavorPointsBonus,
  LastBribedIntimidated,
  LastFlattered,
  MovementNoiseMult,
  BypassVendorStolenCheck,
  BypassVendorKeywordCheck,
  WaitingForPlayer,
  OneHandedMod,
  TwoHandedMod,
  MarksmanMod,
  BlockMod,
  SmithingMod,
  HeavyArmorMod,
  LightArmorMod,
  PickPocketMod,
  LockpickingMod,
  SneakMod,
  AlchemyMod,
  SpeechcraftMod,
  AlterationMod,
  ConjurationMod,
  DestructionMod,
  IllusionMod,
  RestorationMod,
  EnchantingMod,
  OneHandedSkillAdvance,
  TwoHandedSkillAdvance,
  MarksmanSkillAdvance,
  BlockSkillAdvance,
  SmithingSkillAdvance,
  HeavyArmorSkillAdvance,
  LightArmorSkillAdvance,
  PickPocketSkillAdvance,
  LockpickingSkillAdvance,
  SneakSkillAdvance,
  AlchemySkillAdvance,
  SpeechcraftSkillAdvance,
  AlterationSkillAdvance,
  ConjurationSkillAdvance,
  DestructionSkillAdvance,
  IllusionSkillAdvance,
  RestorationSkillAdvance,
  EnchantingSkillAdvance,
  LeftWeaponSpeedMult,
  DragonSouls,
  CombatHealthRegenMult,
  OneHandedPowerMod,
  TwoHandedPowerMod,
  MarksmanPowerMod,
  BlockPowerMod,
  SmithingPowerMod,
  HeavyArmorPowerMod,
  LightArmorPowerMod,
  PickPocketPowerMod,
  LockpickingPowerMod,
  SneakPowerMod,
  AlchemyPowerMod,
  SpeechcraftPowerMod,
  AlterationPowerMod,
  ConjurationPowerMod,
  DestructionPowerMod,
  IllusionPowerMod,
  RestorationPowerMod,
  EnchantingPowerMod,
  DragonRend,
  AttackDamageMult,
  HealRateMult_or_CombatHealthRegenMultMod,
  MagickaRateMult_or_CombatHealthRegenMultPowerMod,
  StaminaRateMult,
  WerewolfPerks,
  VampirePerks,
  GrabActorOffset,
  Grabbed,
  DEPRECATED05,
  ReflectDamage
};

class MGEF : public RecordHeader
{
public:
  static constexpr auto kType = "MGEF";

  struct DATA
  {
    // primary actor value
    ActorValue primaryAV = espm::ActorValue::None;
  };

  struct Data
  {
    DATA data;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(MGEF) == sizeof(RecordHeader));

class ALCH : public RecordHeader
{
public:
  static constexpr auto kType = "ALCH";

  struct Data
  {
    std::vector<espm::Effects::Effect> effects;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(ALCH) == sizeof(RecordHeader));

class INGR : public RecordHeader
{
public:
  static constexpr auto kType = "INGR";

  struct Data
  {
    std::vector<espm::Effects::Effect> effects;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(INGR) == sizeof(RecordHeader));

class BOOK : public RecordHeader
{
public:
  static constexpr auto kType = "BOOK";

  enum Flags : uint8_t
  {
    None = 0,
    TeachesSkill = 0x01,
    CantbeTaken = 0x02,
    TeachesSpell = 0x04,
    AlreadyRead = 0x08,
  };

  struct Data
  {
    [[nodiscard]] bool IsFlagSet(Flags flag) const noexcept;

    Flags flags = Flags::None;
    uint32_t spellOrSkillFormId = 0;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(BOOK) == sizeof(RecordHeader));
}

namespace espm {

class KYWD : public espm::RecordHeader
{
public:
  static constexpr auto kType = "KYWD";

  struct Data
  {
    const char* editorId = "";
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(KYWD) == sizeof(espm::RecordHeader));

uint32_t CalculateHashcode(const void* readBuffer, size_t length);
uint32_t GetCorrectHashcode(const std::string& fileName);

}
#pragma pack(pop)
