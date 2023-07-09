#include "PapyrusForm.h"

#include "EspmGameObject.h"
#include "MpActor.h"
#include "MpFormGameObject.h"
#include "WorldState.h"

#include <string>
#include <unordered_map>

namespace {
std::unordered_map<std::string, int> InitFormTypeMap();
int GetFormType(const std::string& recordName);
}

VarValue PapyrusForm::RegisterForSingleUpdate(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() >= 1) {
    if (auto form = GetFormPtr<MpForm>(self)) {
      double seconds = static_cast<double>(arguments[0]);
      form->GetParent()->RegisterForSingleUpdate(self, seconds);
    }
  }

  return VarValue::None();
}

VarValue PapyrusForm::GetType(VarValue self, const std::vector<VarValue>&)
{
  const auto& selfRec = GetRecordPtr(self);
  if (selfRec.rec) {
    std::string type = selfRec.rec->GetType().ToString();
    return VarValue(static_cast<int32_t>(GetFormType(type)));
  }

  if (auto form = GetFormPtr<MpForm>(self)) {
    if (dynamic_cast<MpActor*>(form)) {
      constexpr auto kCharacter = 62;
      return VarValue(static_cast<int32_t>(kCharacter));
    }
    if (dynamic_cast<MpObjectReference*>(form)) {
      constexpr auto kReference = 61;
      return VarValue(static_cast<int32_t>(kReference));
    }
  }

  return VarValue::None();
}

VarValue PapyrusForm::HasKeyword(VarValue self,
                                 const std::vector<VarValue>& args)
{
  static espm::CompressedFieldsCache g_dummyCache;

  if (args.empty()) {
    spdlog::error("Form.HasKeyword - at least one argument expected");
    return VarValue(false);
  }

  const auto& keywordRec = GetRecordPtr(args[0]);
  if (!keywordRec.rec) {
    spdlog::error("Form.HasKeyword - invalid keyword form");
    return VarValue(false);
  }

  // TODO: check if record type is keyword

  const auto& selfRec = GetRecordPtr(self);
  if (selfRec.rec) {
    auto keywordIds = selfRec.rec->GetKeywordIds(g_dummyCache);
    for (auto rawId : keywordIds) {
      auto globalId = selfRec.ToGlobalId(rawId);
      if (globalId == keywordRec.ToGlobalId(keywordRec.rec->GetId())) {
        return VarValue(true);
      }
    }
  }

  return VarValue(false);
}

namespace {
std::unordered_map<std::string, int> InitFormTypeMap()
{
  std::unordered_map<std::string, int> formTypeMap;

  formTypeMap["GRUP"] = 2;   // group
  formTypeMap["GMST"] = 3;   // gmst
  formTypeMap["KYWD"] = 4;   // keyword
  formTypeMap["AACT"] = 6;   // action
  formTypeMap["TXST"] = 7;   // textureset
  formTypeMap["MICN"] = 8;   // menuicon
  formTypeMap["GLOB"] = 9;   // global
  formTypeMap["CLAS"] = 10;  // class
  formTypeMap["FACT"] = 11;  // faction
  formTypeMap["HDPT"] = 12;  // headpart
  formTypeMap["EYES"] = 13;  // eyes
  formTypeMap["RACE"] = 14;  // race
  formTypeMap["SOUN"] = 15;  // sound
  formTypeMap["ASPC"] = 16;  // acousticspace
  formTypeMap["SKIL"] = 17;  // skill
  formTypeMap["MGEF"] = 18;  // magiceffect
  formTypeMap["SCPT"] = 19;  // script
  formTypeMap["LTEX"] = 20;  // landtexture
  formTypeMap["ENCH"] = 21;  // enchantment
  formTypeMap["SPEL"] = 22;  // spell
  formTypeMap["SCRL"] = 23;  // scrollitem
  formTypeMap["ACTI"] = 24;  // activator
  formTypeMap["TACT"] = 25;  // talkingactivator
  formTypeMap["ARMO"] = 26;  // armor
  formTypeMap["BOOK"] = 27;  // book
  formTypeMap["CONT"] = 28;  // container
  formTypeMap["DOOR"] = 29;  // door
  formTypeMap["INGR"] = 30;  // ingredient
  formTypeMap["LIGH"] = 31;  // light
  formTypeMap["MISC"] = 32;  // miscobject
  formTypeMap["APPA"] = 33;  // apparatus
  formTypeMap["STAT"] = 34;  // static
  formTypeMap["SCOL"] = 35;  // staticcollection
  formTypeMap["MSTT"] = 36;  // movablestatic
  formTypeMap["GRAS"] = 37;  // grass
  formTypeMap["TREE"] = 38;  // tree
  formTypeMap["FLOR"] = 39;  // flora
  formTypeMap["FURN"] = 40;  // furniture
  formTypeMap["WEAP"] = 41;  // weapon
  formTypeMap["AMMO"] = 42;  // ammo
  formTypeMap["NPC_"] = 43;  // actorbase
  formTypeMap["LVLN"] = 44;  // leveledcharacter
  formTypeMap["KEYM"] = 45;  // key
  formTypeMap["ALCH"] = 46;  // potion
  formTypeMap["IDLM"] = 47;  // idlemarker
  formTypeMap["NOTE"] = 48;  // note
  formTypeMap["COBJ"] = 49;  // constructibleobject
  formTypeMap["PROJ"] = 50;  // projectile
  formTypeMap["HAZD"] = 51;  // hazard
  formTypeMap["SLGM"] = 52;  // soulgem
  formTypeMap["LVLI"] = 53;  // leveleditem
  formTypeMap["WTHR"] = 54;  // weather
  formTypeMap["CLMT"] = 55;  // climate
  formTypeMap["SPGD"] = 56;  // spgd
  formTypeMap["RFCT"] = 57;  // rfct
  formTypeMap["REGN"] = 58;  // region
  formTypeMap["NAVI"] = 59;  // navmeshinfo
  formTypeMap["CELL"] = 60;  // cell
  formTypeMap["REFR"] = 61;  // reference
  formTypeMap["ACHR"] = 62;  // actorreference
  formTypeMap["PMIS"] = 63;  // placemarker
  formTypeMap["PARW"] = 64;  // placemarkerreference
  formTypeMap["PGRE"] = 65;  // pgreference
  formTypeMap["PBEA"] = 66;  // pbeareference
  formTypeMap["PFLA"] = 67;  // pflareference
  formTypeMap["PCON"] = 68;  // pconreference
  formTypeMap["PBAR"] = 69;  // pbarreference
  formTypeMap["PHZD"] = 70;  // phzdreference
  formTypeMap["WRLD"] = 71;  // worldspace
  formTypeMap["LAND"] = 72;  // landscape
  formTypeMap["NAVM"] = 73;  // navmesh
  formTypeMap["TLOD"] = 74;  // loddata
  formTypeMap["DIAL"] = 75;  // dialoguetopic
  formTypeMap["INFO"] = 76;  // dialogueinfo
  formTypeMap["QUST"] = 77;  // quest
  formTypeMap["IDLE"] = 78;  // idle
  formTypeMap["PACK"] = 79;  // package
  formTypeMap["CSTY"] = 80;  // combatstyle
  formTypeMap["LSCR"] = 81;  // loadscreen
  formTypeMap["LVSP"] = 82;  // leveledspell
  formTypeMap["ANIO"] = 83;  // animobject
  formTypeMap["WATR"] = 84;  // water
  formTypeMap["EFSH"] = 85;  // efsh
  formTypeMap["EXPL"] = 86;  // explosion
  formTypeMap["DEBR"] = 87;  // debris
  formTypeMap["IMGS"] = 88;  // imagespace
  formTypeMap["IMAD"] = 89;  // imagespacemodifier
  formTypeMap["FLST"] = 90;  // formidlist
  formTypeMap["PERK"] = 91;  // perk
  formTypeMap["BPTD"] = 92;  // bodypartdata
  formTypeMap["ADDN"] = 93;  // addonnode
  formTypeMap["AVIF"] = 94;  // actorvalueinformation
  formTypeMap["CAMS"] = 95;  // camera
  formTypeMap["CPTH"] = 96;  // camerapath
  formTypeMap["VTYP"] = 97;  // voicetype
  formTypeMap["MATT"] = 98;  // materialtype
  formTypeMap["IPCT"] = 99;  // impact
  formTypeMap["IPDS"] = 100; // impactdataset
  formTypeMap["ARMA"] = 101; // armature
  formTypeMap["ECZN"] = 102; // encounterzone
  formTypeMap["LCTN"] = 103; // location

  return formTypeMap;
}

int GetFormType(const std::string& recordName)
{
  static const std::unordered_map<std::string, int> kFormTypeMap =
    InitFormTypeMap();

  auto it = kFormTypeMap.find(recordName);

  if (it != kFormTypeMap.end()) {
    return it->second;
  }

  throw std::runtime_error("Unable to find form type for record name " +
                           recordName);
}
} // namespace
