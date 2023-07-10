#include "EspmGameObject.h"

#include <spdlog/spdlog.h>
#include <sstream>

EspmGameObject::EspmGameObject(const espm::LookupResult& record_)
  : record(record_)
{
}

const char* EspmGameObject::GetParentNativeScript()
{
  if (record.rec) {
    auto t = record.rec->GetType();

    if (t == "GRUP")
      return "group";
    if (t == "GMST")
      return "gmst";
    if (t == "KYWD")
      return "keyword";
    if (t == "LCRT")
      return "locationref";
    if (t == "AACT")
      return "action";
    if (t == "TXST")
      return "textureset";
    if (t == "MICN")
      return "menuicon";
    if (t == "GLOB")
      return "globalvariable";
    if (t == "CLAS")
      return "class";
    if (t == "FACT")
      return "faction";
    if (t == "HDPT")
      return "headpart";
    if (t == "EYES")
      return "eyes";
    if (t == "RACE")
      return "race";
    if (t == "SOUN")
      return "sound";
    if (t == "ASPC")
      return "acousticspace";
    if (t == "SKIL")
      return "skill";
    if (t == "MGEF")
      return "magiceffect";
    if (t == "SCPT")
      return "script";
    if (t == "LTEX")
      return "landtexture";
    if (t == "ENCH")
      return "enchantment";
    if (t == "SPEL")
      return "spell";
    if (t == "SCRL")
      return "scrollitem";
    if (t == "ACTI")
      return "activator";
    if (t == "TACT")
      return "talkingactivator";
    if (t == "ARMO")
      return "armor";
    if (t == "BOOK")
      return "book";
    if (t == "CONT")
      return "container";
    if (t == "DOOR")
      return "door";
    if (t == "INGR")
      return "ingredient";
    if (t == "LIGH")
      return "light";
    if (t == "MISC")
      return "miscobject";
    if (t == "APPA")
      return "apparatus";
    if (t == "STAT")
      return "static";
    if (t == "SCOL")
      return "staticxollection";
    if (t == "MSTT")
      return "movablestatic";
    if (t == "GRAS")
      return "grass";
    if (t == "TREE")
      return "tree";
    if (t == "FLOR")
      return "flora";
    if (t == "FURN")
      return "furniture";
    if (t == "WEAP")
      return "weapon";
    if (t == "AMMO")
      return "ammo";
    if (t == "NPC_")
      return "actorbase";
    if (t == "LVLN")
      return "leveledcharacter";
    if (t == "KEYM")
      return "key";
    if (t == "ALCH")
      return "potion";
    if (t == "IDLM")
      return "idlemarker";
    if (t == "NOTE")
      return "note";
    if (t == "COBJ")
      return "constructibleobject";
    if (t == "PROJ")
      return "projectile";
    if (t == "HAZD")
      return "hazard";
    if (t == "SLGM")
      return "soulgem";
    if (t == "LVLI")
      return "leveleditem";
    if (t == "WTHR")
      return "weather";
    if (t == "CLMT")
      return "climate";
    if (t == "SPGD")
      return "spgd";
    if (t == "RFCT")
      return "referenceeffect";
    if (t == "REGN")
      return "region";
    if (t == "NAVI")
      return "navi";
    if (t == "CELL")
      return "cell";
    if (t == "REFR")
      return "objectreference";
    if (t == "ACHR")
      return "actor";
    if (t == "PMIS")
      return "missile";
    if (t == "PARW")
      return "arrow";
    if (t == "PGRE")
      return "grenade";
    if (t == "PBEA")
      return "neamproj";
    if (t == "PFLA")
      return "flameproj";
    if (t == "PCON")
      return "coneproj";
    if (t == "PBAR")
      return "barrierproj";
    if (t == "PHZD")
      return "hazard";
    if (t == "WRLD")
      return "worldspace";
    if (t == "LAND")
      return "land";
    if (t == "NAVM")
      return "navm";
    if (t == "TLOD")
      return "tlod";
    if (t == "DIAL")
      return "topic";
    if (t == "INFO")
      return "topicinfo";
    if (t == "QUST")
      return "quest";
    if (t == "IDLE")
      return "idle";
    if (t == "PACK")
      return "package";
    if (t == "CSTY")
      return "combatstyle";
    if (t == "LSCR")
      return "loadscreen";
    if (t == "LVSP")
      return "leveledspell";
    if (t == "ANIO")
      return "anio";
    if (t == "WATR")
      return "water";
    if (t == "EFSH")
      return "effectshader";
    if (t == "TOFT")
      return "toft";
    if (t == "EXPL")
      return "explosion";
    if (t == "DEBR")
      return "debris";
    if (t == "IMGS")
      return "imagespace";
    if (t == "IMAD")
      return "imagespacemodifier";
    if (t == "FLST")
      return "formlist";
    if (t == "PERK")
      return "Perk";
    if (t == "BPTD")
      return "bodypartdata";
    if (t == "ADDN")
      return "addonnode";
    if (t == "AVIF")
      return "actorvalueinfo";
    if (t == "CAMS")
      return "camerashot";
    if (t == "CPTH")
      return "camerapath";
    if (t == "VTYP")
      return "voicetype";
    if (t == "MATT")
      return "materialtype";
    if (t == "IPCT")
      return "impactdata";
    if (t == "IPDS")
      return "impactdataset";
    if (t == "ARMA")
      return "arma";
    if (t == "ECZN")
      return "encounterzone";
    if (t == "LCTN")
      return "location";
    if (t == "MESG")
      return "message";
    if (t == "RGDL")
      return "ragdoll";
    if (t == "DOBJ")
      return "dobj";
    if (t == "LGTM")
      return "lightingtemplate";
    if (t == "MUSC")
      return "musictype";
    if (t == "FSTP")
      return "footstep";
    if (t == "FSTS")
      return "footstepset";
    if (t == "SMBN")
      return "storybranchnode";
    if (t == "SMQN")
      return "storyquestnode";
    if (t == "SMEN")
      return "storyeventnode";
    if (t == "DLBR")
      return "dialoguebranch";
    if (t == "MUST")
      return "musictrack";
    if (t == "DLVW")
      return "dlvw";
    if (t == "WOOP")
      return "wordofpower";
    if (t == "SHOU")
      return "shout";
    if (t == "EQUP")
      return "equipslot";
    if (t == "RELA")
      return "relationship";
    if (t == "SCEN")
      return "scene";
    if (t == "ASTP")
      return "associationtype";
    if (t == "OTFT")
      return "outfit";
    if (t == "ARTO")
      return "art";
    if (t == "MATO")
      return "material";
    if (t == "MOVT")
      return "movementtype";
    if (t == "SNDR")
      return "sounddescriptor";
    if (t == "DUAL")
      return "dualcastdata";
    if (t == "SNCT")
      return "soundcategory";
    if (t == "SOPM")
      return "soundoutput";
    if (t == "COLL")
      return "collisionlayer";
    if (t == "CLFM")
      return "colorform";
    if (t == "REVB")
      return "reverbparam";

    // P.S. there is some code outside espm that relies on REFR/ACHR

    throw std::runtime_error("Unable to find native script for record type " +
                             t.ToString());
  }
  return "";
}

bool EspmGameObject::EqualsByValue(const IGameObject& obj) const
{
  if (auto espmObj = dynamic_cast<const EspmGameObject*>(&obj)) {
    return espmObj->record.rec == record.rec;
  }
  return false;
}

const espm::LookupResult& GetRecordPtr(const VarValue& papyrusObject)
{
  static const espm::LookupResult emptyResult;

  if (papyrusObject.GetType() != VarValue::kType_Object) {
    std::stringstream papyrusObjectStr;
    papyrusObjectStr << papyrusObject;
    spdlog::warn("GetRecordPtr called with non-object ({})",
                 papyrusObjectStr.str());
    return emptyResult;
  }
  auto gameObject = static_cast<IGameObject*>(papyrusObject);
  if (!gameObject) {
    std::stringstream papyrusObjectStr;
    papyrusObjectStr << papyrusObject;
    spdlog::warn("GetRecordPtr called with null object ({})",
                 papyrusObjectStr.str());
    return emptyResult;
  }
  auto espmGameObject = dynamic_cast<EspmGameObject*>(gameObject);
  if (!espmGameObject) {
    std::stringstream papyrusObjectStr;
    papyrusObjectStr << papyrusObject;
    spdlog::warn("GetRecordPtr called with non-espm object ({})",
                 papyrusObjectStr.str());
    return emptyResult;
  }
  return espmGameObject->record;
}

const char* EspmGameObject::GetStringID()
{
  static std::map<uint32_t, std::shared_ptr<std::string>> g_strings;
  auto formId = this->record.ToGlobalId(this->record.rec->GetId());
  auto& v = g_strings[formId];
  if (!v) {
    v.reset(new std::string(fmt::format("espm {:x}", formId)));
  }
  return v->data();
}
