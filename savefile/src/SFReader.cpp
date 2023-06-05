#include "savefile/SFReader.h"

#include <cassert>
#include <fstream>
#include <iostream>

void SaveFile_::Reader::Read()
{
  char temp;

  arrayBytes.clear();

  std::ifstream File(path, std::ios::binary);

  if (File.is_open()) {

    File.seekg(0, std::ios_base::end);
    auto fileSize = File.tellg();
    File.seekg(0, std::ios_base::beg);

    std::cout << "File size (in bytes): " << fileSize << std::endl;

    while (File.get(temp)) {
      arrayBytes.push_back(temp);
    }

  } else {
    throw std::runtime_error("Error open file: " + path);
  }

  File.close();
};

SaveFile_::Reader::Reader(std::string path)
{
  this->currentReadPositionInFile = 0;
  this->path = path;
  Read();
  CreateScriptStructure(arrayBytes);
}

SaveFile_::Reader::Reader(const uint8_t* data, size_t size)
{
  CreateScriptStructure({ data, data + size });
}

void SaveFile_::Reader::CreateScriptStructure(std::vector<uint8_t> arrayBytes)
{
  this->arrayBytes = arrayBytes;
  currentReadPositionInFile = 0;

  this->structure = std::make_shared<SaveFile>(SaveFile());

  structure->magic = ReadString(13);
  structure->headerSize = ReadUint32_bit();
  structure->header = FillHeader();

  const size_t sizeScreenData =
    structure->header.shotWidth * structure->header.shotHeight * 3;
  structure->screenshotData.resize(sizeScreenData);

  for (auto& byte : structure->screenshotData)
    byte = Read8_bit();

  structure->formVersion = Read8_bit();
  structure->pluginInfoSize = ReadUint32_bit();
  structure->pluginInfo = FillPluginInfo();

  structure->fileLocationTable = FillFileLocationTable();

  assert(this->currentReadPositionInFile ==
         structure->fileLocationTable.globalDataTable1Offset);
  structure->globalDataTable1 =
    FillGlobalData(structure->fileLocationTable.globalDataTable1Count);

  assert(this->currentReadPositionInFile ==
         structure->fileLocationTable.globalDataTable2Offset);
  structure->globalDataTable2 =
    FillGlobalData(structure->fileLocationTable.globalDataTable2Count);

  assert(this->currentReadPositionInFile ==
         structure->fileLocationTable.changeFormsOffset);
  structure->changeForms =
    FillChangeForm(structure->fileLocationTable.changeFormCount);

  assert(this->currentReadPositionInFile ==
         structure->fileLocationTable.globalDataTable3Offset);
  structure->globalDataTable3 =
    FillGlobalData(structure->fileLocationTable.globalDataTable3Count);

  structure->fixForBag =
    Read64_bit(); /// fix for next assert , globalDataTable3Count is currently
                  /// bugged (as of version 112) that it does not include type
                  /// 1001 (Papyrus) in the count.

  assert(this->currentReadPositionInFile ==
         structure->fileLocationTable.formIDArrayCountOffset);
  structure->formIDArrayCount = ReadUint32_bit();
  structure->formIDArray.resize(structure->formIDArrayCount);
  for (auto& formID : structure->formIDArray)
    formID = ReadUint32_bit();

  structure->visitedWorldspaceArrayCount = ReadUint32_bit();
  structure->visitedWorldspaceArray.resize(
    structure->visitedWorldspaceArrayCount);
  for (auto& formID : structure->visitedWorldspaceArray)
    formID = ReadUint32_bit();

  assert(this->currentReadPositionInFile ==
         structure->fileLocationTable.unknownTable3Offset);
  structure->unknown3TableSize = ReadUint32_bit();
  structure->unknown3Table = FillUnknown3Table();
}

SaveFile_::Header SaveFile_::Reader::FillHeader()
{
  Header header;

  header.version = ReadUint32_bit();
  header.saveNumber = ReadUint32_bit();
  header.playerName = ReadString(Read16_bit());
  header.playerLevel = ReadUint32_bit();
  header.playerLocation = ReadString(Read16_bit());
  header.gameDate = ReadString(Read16_bit());
  header.playerRaceEditorId = ReadString(Read16_bit());
  header.playerSex = Read16_bit();
  header.playerCurExp = ReadFloat32_bit();
  header.playerLvlUpExp = ReadFloat32_bit();

  for (auto& byte : header.filetime)
    byte = Read8_bit();

  header.shotWidth = ReadUint32_bit();
  header.shotHeight = ReadUint32_bit();

  return header;
}

SaveFile_::PluginInfo SaveFile_::Reader::FillPluginInfo()
{
  PluginInfo pluginInfo;

  pluginInfo.numPlugins = Read8_bit();
  pluginInfo.pluginsName.resize(pluginInfo.numPlugins);

  for (auto& name : pluginInfo.pluginsName)
    name = ReadString(Read16_bit());

  return pluginInfo;
}

SaveFile_::FileLocationTable SaveFile_::Reader::FillFileLocationTable()
{
  FileLocationTable fileLocationTable;

  fileLocationTable.formIDArrayCountOffset = ReadUint32_bit();
  fileLocationTable.unknownTable3Offset = ReadUint32_bit();
  fileLocationTable.globalDataTable1Offset = ReadUint32_bit();
  fileLocationTable.globalDataTable2Offset = ReadUint32_bit();
  fileLocationTable.changeFormsOffset = ReadUint32_bit();
  fileLocationTable.globalDataTable3Offset = ReadUint32_bit();
  fileLocationTable.globalDataTable1Count = ReadUint32_bit();
  fileLocationTable.globalDataTable2Count = ReadUint32_bit();
  fileLocationTable.globalDataTable3Count = ReadUint32_bit();
  fileLocationTable.changeFormCount = ReadUint32_bit();

  for (auto& number : fileLocationTable.unused)
    number = ReadUint32_bit();

  return fileLocationTable;
}

std::vector<SaveFile_::GlobalData> SaveFile_::Reader::FillGlobalData(
  uint32_t numObject)
{
  std::vector<GlobalData> globalData;
  globalData.resize(numObject);

  for (auto& gData : globalData) {
    gData.type = ReadUint32_bit();
    gData.length = ReadUint32_bit();

    uint32_t stepAfterReadData =
      gData.length + this->currentReadPositionInFile;

    switch (gData.type) {
      case 0:
        FillMiscStats(gData);
        break;
      case 1:
        FillPlayerLocation(gData);
        break;
      case 2:
        FillTES(gData);
        break;
      case 3:
        FillGlobalVariables(gData);
        break;
      case 4:
        FillCreatedObjects(gData);
        break;
      case 5:
        FillEffects(gData);
        break;
      case 6:
        FillWeather(gData);
        break;
      case 7:
        FillAudio(gData);
        break;
      case 8:
        FillSkyCells(gData);
        break;
      case 100:
        FillProcessList(gData);
        break;
      case 101:
        FillCombat(gData);
        break;
      case 102:
        FillInterface(gData);
        break;
      case 103:
        FillActorCauses(gData);
        break;
      case 104:
        FillMain(gData); /// TODO Unknown format.
        break;
      case 105:
        FillDetectionManager(gData);
        break;
      case 106:
        FillLocationMetaData(gData);
        break;
      case 107:
        FillQuestStaticData(gData);
        break;
      case 108:
        FillStoryTeller(gData);
        break;
      case 109:
        FillMagicFavorites(gData);
        break;
      case 110:
        FillPlayerControls(gData);
        break;
      case 111:
        FillMain(gData); /// TODO Unknown format.
        break;
      case 112:
        FillIngredientShared(gData);
        break;
      case 113:
        FillMenuControls(gData);
        break;
      case 114:
        FillMenuTopicManager(gData);
        break;
      case 1000:
        FillMain(gData); /// TODO
        break;
      case 1001:
        FillMain(gData); /// TODO
        break;
      case 1002:
        FillAnimObjects(gData);
        break;
      case 1003:
        FillTimer(gData);
        break;
      case 1004:
        FillMain(gData); /// TODO Unknown format.
        break;
      case 1005:
        FillMain(gData); /// TODO Always Empty.
        break;
    }
    assert(stepAfterReadData == this->currentReadPositionInFile);
  }

  return globalData;
}

std::vector<SaveFile_::ChangeForm> SaveFile_::Reader::FillChangeForm(
  uint32_t numObject)
{
  std::vector<ChangeForm> changeForm;
  changeForm.resize(numObject);

  for (auto& form : changeForm) {
    form.formID = FillRefID();
    form.changeFlags = ReadUint32_bit();
    form.type = Read8_bit();
    form.version = Read8_bit();

    if (form.type > 0x3F && form.type <= 0x7F) {
      form.length1 = Read16_bit();
      form.length2 = Read16_bit();
    }

    if (form.type > 0x7F) {
      form.length1 = ReadUint32_bit();
      form.length2 = ReadUint32_bit();
    }

    if (form.type <= 0x3F) {
      form.length1 = Read8_bit();
      form.length2 = Read8_bit();
    }

    form.data.resize(form.length1);

    for (auto& byte : form.data)
      byte = Read8_bit();
  }

  return changeForm;
}

SaveFile_::RefID SaveFile_::Reader::FillRefID()
{
  RefID formID;

  formID.byte0 = Read8_bit();
  formID.byte1 = Read8_bit();
  formID.byte2 = Read8_bit();

  return formID;
}

SaveFile_::Unknown3Table SaveFile_::Reader::FillUnknown3Table()
{
  Unknown3Table unknown3Table;

  unknown3Table.count = ReadUint32_bit();
  unknown3Table.unknown.resize(unknown3Table.count);
  for (auto& string : unknown3Table.unknown)
    string = ReadString(Read16_bit());

  return unknown3Table;
}

void SaveFile_::Reader::FillMiscStats(GlobalData& globalData)
{
  MiscStats miscStats;
  miscStats.numStats = ReadUint32_bit();
  miscStats.stats.resize(miscStats.numStats);

  for (auto& stat : miscStats.stats) {
    stat.name = ReadString(Read16_bit());
    stat.category = Read8_bit();

    if (stat.category < 0 || stat.category > 6)
      throw std::runtime_error("unknown category!");

    stat.value = ReadInt32_bit();
  }

  globalData.data = std::make_shared<MiscStats>(miscStats);
}

void SaveFile_::Reader::FillPlayerLocation(GlobalData& globalData)
{
  PlayerLocation playerLocation;
  playerLocation.nextObjectId = ReadUint32_bit();
  playerLocation.worldspace1 = FillRefID();
  playerLocation.coorX = ReadInt32_bit();
  playerLocation.coorY = ReadInt32_bit();
  playerLocation.worldspace2 = FillRefID();
  playerLocation.posX = ReadFloat32_bit();
  playerLocation.posY = ReadFloat32_bit();
  playerLocation.posZ = ReadFloat32_bit();
  playerLocation.unknown = Read8_bit();

  globalData.data = std::make_shared<PlayerLocation>(playerLocation);
}

void SaveFile_::Reader::FillTES(GlobalData& globalData)
{
  SaveFile_::TES tes;

  tes.numUnknown1 = ReadVsval_bit();
  tes.unknowns1.resize(tes.numUnknown1);

  for (auto& unc : tes.unknowns1) {
    unc.formID = FillRefID();
    unc.unknown = Read16_bit();
  }

  tes.numUnknown2 = ReadUint32_bit();
  tes.unknowns2.resize(tes.numUnknown2 * tes.numUnknown2);

  for (auto& unc : tes.unknowns2)
    unc = FillRefID();

  tes.numUnknown3 = ReadVsval_bit();
  tes.unknowns3.resize(tes.numUnknown3);

  for (auto& unc : tes.unknowns3)
    unc = FillRefID();

  globalData.data = std::make_shared<TES>(tes);
}

void SaveFile_::Reader::FillGlobalVariables(GlobalData& globalData)
{
  GlobalVariables globalVariables;

  globalVariables.numGlobals = ReadVsval_bit();
  globalVariables.globals.resize(globalVariables.numGlobals);

  for (auto& gv : globalVariables.globals) {
    gv.formID = FillRefID();
    gv.value = ReadFloat32_bit();
  }

  globalData.data = std::make_shared<GlobalVariables>(globalVariables);
}

void SaveFile_::Reader::FillCreatedObjects(GlobalData& globalData)
{
  CreatedObjects createdObjects;

  createdObjects.numWeapon = ReadVsval_bit();
  createdObjects.weaponEnchTable.resize(createdObjects.numWeapon);

  for (auto& weaponEnch : createdObjects.weaponEnchTable) {
    weaponEnch.refID = FillRefID();
    weaponEnch.timesUsed = ReadUint32_bit();
    weaponEnch.numEffects = ReadVsval_bit();
    weaponEnch.effects.resize(weaponEnch.numEffects);

    for (auto& effect : weaponEnch.effects) {
      effect.effectID = FillRefID();
      effect.info.magnitude = ReadFloat32_bit();
      effect.info.duration = ReadUint32_bit();
      effect.info.area = ReadUint32_bit();
      effect.price = ReadFloat32_bit();
    }
  }

  createdObjects.numArmor = ReadVsval_bit();
  createdObjects.armourEnchTable.resize(createdObjects.numArmor);

  for (auto& armourEnch : createdObjects.armourEnchTable) {
    armourEnch.refID = FillRefID();
    armourEnch.timesUsed = ReadUint32_bit();
    armourEnch.numEffects = ReadVsval_bit();
    armourEnch.effects.resize(armourEnch.numEffects);

    for (auto& effect : armourEnch.effects) {
      effect.effectID = FillRefID();
      effect.info.magnitude = ReadFloat32_bit();
      effect.info.duration = ReadUint32_bit();
      effect.info.area = ReadUint32_bit();
      effect.price = ReadFloat32_bit();
    }
  }

  createdObjects.numPotion = ReadVsval_bit();
  createdObjects.potionTable.resize(createdObjects.numPotion);

  for (auto& potion : createdObjects.potionTable) {
    potion.refID = FillRefID();
    potion.timesUsed = ReadUint32_bit();
    potion.numEffects = ReadVsval_bit();
    potion.effects.resize(potion.numEffects);

    for (auto& effect : potion.effects) {
      effect.effectID = FillRefID();
      effect.info.magnitude = ReadFloat32_bit();
      effect.info.duration = ReadUint32_bit();
      effect.info.area = ReadUint32_bit();
      effect.price = ReadFloat32_bit();
    }
  }

  createdObjects.numPoison = ReadVsval_bit();
  createdObjects.poisonTable.resize(createdObjects.numPoison);

  for (auto& poison : createdObjects.poisonTable) {
    poison.refID = FillRefID();
    poison.timesUsed = ReadUint32_bit();
    poison.numEffects = ReadVsval_bit();
    poison.effects.resize(poison.numEffects);

    for (auto& effect : poison.effects) {
      effect.effectID = FillRefID();
      effect.info.magnitude = ReadFloat32_bit();
      effect.info.duration = ReadUint32_bit();
      effect.info.area = ReadUint32_bit();
      effect.price = ReadFloat32_bit();
    }
  }

  globalData.data = std::make_shared<CreatedObjects>(createdObjects);
}

void SaveFile_::Reader::FillEffects(GlobalData& globalData)
{
  Effects effects;

  effects.numImgSpaceMod = ReadVsval_bit();
  effects.imageSpaceModifiers.resize(effects.numImgSpaceMod);

  for (auto& imgSM : effects.imageSpaceModifiers) {
    imgSM.strength = ReadFloat32_bit();
    imgSM.timestamp = ReadFloat32_bit();
    imgSM.unknown = ReadUint32_bit();
    imgSM.effectID = FillRefID();
  }

  effects.unknown1 = ReadFloat32_bit();
  effects.unknown2 = ReadFloat32_bit();

  globalData.data = std::make_shared<Effects>(effects);
}

void SaveFile_::Reader::FillWeather(GlobalData& globalData)
{
  Weather weather;

  weather.climate = FillRefID();
  weather.weather = FillRefID();
  weather.prevWeather = FillRefID();
  weather.unknownWeather1 = FillRefID();
  weather.unknownWeather2 = FillRefID();
  weather.regnWeather = FillRefID();
  weather.curTime = ReadFloat32_bit();
  weather.begTime = ReadFloat32_bit();
  weather.weatherPct = ReadFloat32_bit();
  weather.unknown1 = ReadUint32_bit();
  weather.unknown2 = ReadUint32_bit();
  weather.unknown3 = ReadUint32_bit();
  weather.unknown4 = ReadUint32_bit();
  weather.unknown5 = ReadUint32_bit();
  weather.unknown6 = ReadUint32_bit();
  weather.unknown7 = ReadFloat32_bit();
  weather.unknown8 = ReadUint32_bit();
  weather.flags = Read8_bit();

  assert((weather.flags | 0b11111110) != 0b11111111);
  // weather.unknown9 = Only present if (weather.flags | 0b11111110) ==
  // 0b11111111)

  assert((weather.flags | 0b11111101) != 0b11111111);
  // weather.unknown10 = Only present if (weather.flags | 0b11111101) ==
  // 0b11111111)

  globalData.data = std::make_shared<Weather>(weather);
}

void SaveFile_::Reader::FillAudio(GlobalData& globalData)
{
  Audio audio;

  audio.unknown = FillRefID();
  audio.numTracks = ReadVsval_bit();
  audio.tracks.resize(audio.numTracks);

  for (auto& treack : audio.tracks)
    treack = FillRefID();

  audio.bgm = FillRefID();

  globalData.data = std::make_shared<Audio>(audio);
}

void SaveFile_::Reader::FillSkyCells(GlobalData& globalData)
{
  SkyCells skyCells;

  skyCells.numUnknown = ReadVsval_bit();
  skyCells.unknowns.resize(skyCells.numUnknown);

  for (auto& unc : skyCells.unknowns) {
    unc.unknown1 = FillRefID();
    unc.unknown2 = FillRefID();
  }

  globalData.data = std::make_shared<SkyCells>(skyCells);
}

void SaveFile_::Reader::FillProcessList(GlobalData& globalData)
{
  ProcessList processList;

  processList.unknown1 = ReadFloat32_bit();
  processList.unknown2 = ReadFloat32_bit();
  processList.unknown3 = ReadFloat32_bit();
  processList.numNext = ReadUint32_bit();

  for (auto& crimeType : processList.allCrimeTypes) {
    crimeType.numCrimes = ReadVsval_bit();
    crimeType.crimes.resize(crimeType.numCrimes);

    for (auto& crime : crimeType.crimes) {
      crime.witnessNum = ReadUint32_bit();
      crime.crimeType = ReadUint32_bit();
      crime.unknown1 = Read8_bit();
      crime.quantity = ReadUint32_bit();
      crime.numSerial = ReadUint32_bit();
      crime.unknown2 = Read8_bit();
      crime.unknown3 = ReadUint32_bit();
      crime.elapsedTime = ReadFloat32_bit();
      crime.victimID = FillRefID();
      crime.criminalID = FillRefID();
      crime.itemBaseID = FillRefID();
      crime.ownershipID = FillRefID();
      crime.numWitnesses = ReadVsval_bit();
      crime.witnesses.resize(crime.numWitnesses);

      for (auto& witness : crime.witnesses)
        witness = FillRefID();

      crime.bounty = ReadUint32_bit();
      crime.crimeFactionID = FillRefID();
      crime.isCleared = Read8_bit();
      crime.unknown4 = Read16_bit();
    }
  }

  globalData.data = std::make_shared<ProcessList>(processList);
}

void SaveFile_::Reader::FillCombat(GlobalData& globalData)
{
  Combat combat;

  combat.nextNum = ReadUint32_bit();
  combat.numUnknown0 = ReadVsval_bit();
  combat.unknowns0.resize(combat.numUnknown0);

  for (auto& unk0 : combat.unknowns0) {
    unk0.unknown1 = ReadUint32_bit();
    unk0.serialNum = ReadUint32_bit();

    auto& unk0_2 = unk0.unknown2;

    unk0_2.numUnknown0 = ReadVsval_bit();
    unk0_2.unknowns0.resize(unk0_2.numUnknown0);

    for (auto& unk0_2_0 : unk0_2.unknowns0) {
      unk0_2_0.unknown1 = FillRefID();
      unk0_2_0.unknown2 = ReadUint32_bit();
      unk0_2_0.unknown3 = ReadFloat32_bit();
      unk0_2_0.unknown4 = Read16_bit();
      unk0_2_0.unknown5 = Read16_bit();
      unk0_2_0.target = FillRefID();
      unk0_2_0.unknown6.x = ReadFloat32_bit();
      unk0_2_0.unknown6.y = ReadFloat32_bit();
      unk0_2_0.unknown6.z = ReadFloat32_bit();
      unk0_2_0.unknown6.cellID = FillRefID();
      unk0_2_0.unknown7.x = ReadFloat32_bit();
      unk0_2_0.unknown7.y = ReadFloat32_bit();
      unk0_2_0.unknown7.z = ReadFloat32_bit();
      unk0_2_0.unknown7.cellID = FillRefID();
      unk0_2_0.unknown8.x = ReadFloat32_bit();
      unk0_2_0.unknown8.y = ReadFloat32_bit();
      unk0_2_0.unknown8.z = ReadFloat32_bit();
      unk0_2_0.unknown8.cellID = FillRefID();
      unk0_2_0.unknown9.x = ReadFloat32_bit();
      unk0_2_0.unknown9.y = ReadFloat32_bit();
      unk0_2_0.unknown9.z = ReadFloat32_bit();
      unk0_2_0.unknown9.cellID = FillRefID();
      unk0_2_0.unknown10.x = ReadFloat32_bit();
      unk0_2_0.unknown10.y = ReadFloat32_bit();
      unk0_2_0.unknown10.z = ReadFloat32_bit();
      unk0_2_0.unknown10.cellID = FillRefID();
      unk0_2_0.unknown11 = ReadFloat32_bit();
      unk0_2_0.unknown12 = ReadFloat32_bit();
      unk0_2_0.unknown13 = ReadFloat32_bit();
      unk0_2_0.unknown14 = ReadFloat32_bit();
      unk0_2_0.unknown15 = ReadFloat32_bit();
      unk0_2_0.unknown16 = ReadFloat32_bit();
    }

    unk0_2.numUnknown1 = ReadVsval_bit();
    unk0_2.unknowns1.resize(unk0_2.numUnknown1);

    for (auto& unk0_2_1 : unk0_2.unknowns1) {
      unk0_2_1.unknown1 = FillRefID();
      unk0_2_1.unknown2 = ReadFloat32_bit();
      unk0_2_1.unknown3 = ReadFloat32_bit();
    }

    unk0_2.unknown2.unknown1 = ReadFloat32_bit();
    unk0_2.unknown2.unknown2 = ReadFloat32_bit();
    unk0_2.unknown3.unknown1 = ReadFloat32_bit();
    unk0_2.unknown3.unknown2 = ReadFloat32_bit();
    unk0_2.unknown4.unknown1 = ReadFloat32_bit();
    unk0_2.unknown4.unknown2 = ReadFloat32_bit();
    unk0_2.unknown5.unknown1 = ReadFloat32_bit();
    unk0_2.unknown5.unknown2 = ReadFloat32_bit();

    for (auto& unk0_2_6 : unk0_2.unknowns6) {
      unk0_2_6.unknown1 = ReadFloat32_bit();
      unk0_2_6.unknown2 = ReadFloat32_bit();
    }

    unk0_2.unknownFlag = ReadUint32_bit();

    FillUnknown0_0_2(unk0_2.unknownFlag, unk0);

    unk0_2.unknown8.unknown1 = ReadFloat32_bit();
    unk0_2.unknown8.unknown2 = ReadFloat32_bit();
    unk0_2.unknown9 = ReadFloat32_bit();
    unk0_2.unknown10 = ReadFloat32_bit();
    unk0_2.unknown11 = ReadFloat32_bit();
    unk0_2.unknown12 = ReadFloat32_bit();
    unk0_2.unknown13.unknown1 = ReadFloat32_bit();
    unk0_2.unknown13.unknown2 = ReadFloat32_bit();
    unk0_2.unknown14 = Read8_bit();
  }

  combat.numUnknown1 = ReadVsval_bit();
  combat.unknowns1.resize(combat.numUnknown1);

  for (auto& unk1 : combat.unknowns1) {
    unk1.unknown1 = FillRefID();
    unk1.unknown2 = ReadFloat32_bit();
    unk1.unknown3 = FillRefID();
    unk1.unknown4 = FillRefID();
    unk1.unknown5 = ReadFloat32_bit();
    unk1.unknown6 = ReadFloat32_bit();
    unk1.unknown7 = ReadFloat32_bit();
    unk1.x = ReadFloat32_bit();
    unk1.y = ReadFloat32_bit();
    unk1.z = ReadFloat32_bit();
    unk1.unknown8 = ReadFloat32_bit();
    unk1.unknown9 = ReadFloat32_bit();
    unk1.unknown10 = ReadFloat32_bit();
    unk1.unknown11 = ReadFloat32_bit();
    unk1.unknown12 = ReadFloat32_bit();
    unk1.unknown13 = ReadFloat32_bit();
    unk1.unknown14 = ReadFloat32_bit();
    unk1.unknown15 = ReadFloat32_bit();
    unk1.unknown16 = FillRefID();
  }

  combat.unknown2 = ReadFloat32_bit();
  combat.unknown3 = ReadVsval_bit();
  combat.numUnknown4 = ReadVsval_bit();
  combat.unknowns4.resize(combat.numUnknown4);

  for (auto& unk4 : combat.unknowns4)
    unk4 = FillRefID();

  combat.unknown5 = ReadFloat32_bit();
  combat.unknown6.unknown1 = ReadFloat32_bit();
  combat.unknown6.unknown2 = ReadFloat32_bit();
  combat.unknown7.unknown1 = ReadFloat32_bit();
  combat.unknown7.unknown2 = ReadFloat32_bit();

  globalData.data = std::make_shared<Combat>(combat);
}

void SaveFile_::Reader::FillUnknown0_0_2(const uint32_t& flag,
                                         Combat::Unknown0& unk0)
{
  if (flag != 0) {

    auto& unk0_2_7 = unk0.unknown2.unknown7;

    unk0_2_7.unknown1 = FillRefID();
    unk0_2_7.unknown2.unknown1 = ReadFloat32_bit();
    unk0_2_7.unknown2.unknown2 = ReadFloat32_bit();
    unk0_2_7.unknown3.unknown1 = ReadFloat32_bit();
    unk0_2_7.unknown3.unknown2 = ReadFloat32_bit();
    unk0_2_7.unknown4 = ReadFloat32_bit();
    unk0_2_7.unknown5.x = ReadFloat32_bit();
    unk0_2_7.unknown5.y = ReadFloat32_bit();
    unk0_2_7.unknown5.z = ReadFloat32_bit();
    unk0_2_7.unknown5.cellID = FillRefID();
    unk0_2_7.unknown6 = ReadFloat32_bit();
    unk0_2_7.numUnknown7 = ReadVsval_bit();
    unk0_2_7.unknowns7.resize(unk0_2_7.numUnknown7);

    for (auto& unk0_2_7_7 : unk0_2_7.unknowns7) {
      unk0_2_7_7.unknown1.x = ReadFloat32_bit();
      unk0_2_7_7.unknown1.y = ReadFloat32_bit();
      unk0_2_7_7.unknown1.z = ReadFloat32_bit();
      unk0_2_7_7.unknown1.cellID = FillRefID();
      unk0_2_7_7.unknown2 = ReadUint32_bit();
      unk0_2_7_7.unknown3 = ReadFloat32_bit();
    }

    unk0_2_7.numUnknown8 = ReadVsval_bit();
    unk0_2_7.unknowns8.resize(unk0_2_7.numUnknown8);

    for (auto& unk0_2_7_8 : unk0_2_7.unknowns8) {
      unk0_2_7_8.unknown1 = FillRefID();
      unk0_2_7_8.unknown2 = FillRefID();
      unk0_2_7_8.unknown3 = Read8_bit();
      unk0_2_7_8.unknown4 = Read8_bit();
      unk0_2_7_8.unknown5 = Read8_bit();
    }

    unk0_2_7.unknownFlag = Read8_bit();

    if (unk0_2_7.unknownFlag != 0) {

      auto& unk0_2_7_9 = unk0_2_7.unknown9;

      unk0_2_7_9.unknown1 = ReadUint32_bit();
      unk0_2_7_9.unknown2 = ReadUint32_bit();
      unk0_2_7_9.numUnknown3 = ReadUint32_bit();
      unk0_2_7_9.unknowns3.resize(unk0_2_7_9.numUnknown3);

      for (auto& unk0_2_7_9_3 : unk0_2_7_9.unknowns3) {
        unk0_2_7_9_3.unknown1 = Read8_bit();
        unk0_2_7_9_3.numUnknown2 = ReadUint32_bit();
        unk0_2_7_9_3.unknowns2.resize(unk0_2_7_9_3.numUnknown2);

        for (auto& unk0_2_7_9_3_2 : unk0_2_7_9_3.unknowns2)
          unk0_2_7_9_3_2 = Read8_bit();

        unk0_2_7_9_3.unknown3 = FillRefID();
        unk0_2_7_9_3.unknown4 = ReadUint32_bit();
      }

      unk0_2_7_9.unknown4 = FillRefID();
      unk0_2_7_9.unknown5 = ReadFloat32_bit();
      unk0_2_7_9.unknown6 = ReadFloat32_bit();
      unk0_2_7_9.unknown7 = ReadFloat32_bit();
      unk0_2_7_9.unknown8 = ReadFloat32_bit();
      unk0_2_7_9.unknown9 = ReadFloat32_bit();
      unk0_2_7_9.unknown10 = FillRefID();
      unk0_2_7_9.unknown11 = ReadFloat32_bit();
      unk0_2_7_9.unknown12 = FillRefID();
      unk0_2_7_9.unknown13 = Read16_bit();
      unk0_2_7_9.unknown14 = Read8_bit();
      unk0_2_7_9.unknown15 = Read8_bit();
      unk0_2_7_9.unknown16 = ReadFloat32_bit();
      unk0_2_7_9.unknown17 = ReadFloat32_bit();
    }
  }
}

void SaveFile_::Reader::FillInterface(GlobalData& globalData)
{
  Interface _interface;

  _interface.numShownHelpMsg = ReadUint32_bit();
  _interface.shownHelpMsg.resize(_interface.numShownHelpMsg);

  for (auto& msg : _interface.shownHelpMsg)
    msg = ReadUint32_bit();

  _interface.unknown1 = Read8_bit();
  _interface.numLastUsedWeapons = ReadVsval_bit();
  _interface.lastUsedWeapons.resize(_interface.numLastUsedWeapons);

  for (auto& weap : _interface.lastUsedWeapons)
    weap = FillRefID();

  _interface.numLastUsedSpells = ReadVsval_bit();
  _interface.lastUsedSpells.resize(_interface.numLastUsedSpells);

  for (auto& spell : _interface.lastUsedSpells)
    spell = FillRefID();

  _interface.numLastUsedShouts = ReadVsval_bit();
  _interface.lastUsedShouts.resize(_interface.numLastUsedShouts);

  for (auto& shout : _interface.lastUsedShouts)
    shout = FillRefID();

  _interface.unknown2 = Read8_bit();
  _interface.unknown3.numUnknown1 = ReadVsval_bit();
  _interface.unknown3.unknowns1.resize(_interface.unknown3.numUnknown1);

  for (auto& unk3_1 : _interface.unknown3.unknowns1) {
    unk3_1.unknown1 = ReadString(Read16_bit());
    unk3_1.unknown2 = ReadString(Read16_bit());
    unk3_1.unknown3 = ReadUint32_bit();
    unk3_1.unknown4 = ReadUint32_bit();
    unk3_1.unknown5 = ReadUint32_bit();
    unk3_1.unknown6 = ReadUint32_bit();
  }

  _interface.unknown3.numUnknown2 = ReadVsval_bit();
  _interface.unknown3.unknowns2.resize(_interface.unknown3.numUnknown2);

  for (auto& unk3_2 : _interface.unknown3.unknowns2)
    unk3_2 = ReadString(Read16_bit());

  _interface.unknown3.unknown3 = ReadUint32_bit();

  globalData.data = std::make_shared<Interface>(_interface);
}

void SaveFile_::Reader::FillActorCauses(GlobalData& globalData)
{
  ActorCauses actorCauses;

  actorCauses.nextNum = ReadUint32_bit();
  actorCauses.numUnknown = ReadVsval_bit();
  actorCauses.unknowns.resize(actorCauses.numUnknown);

  for (auto& unk : actorCauses.unknowns) {
    unk.x = ReadFloat32_bit();
    unk.y = ReadFloat32_bit();
    unk.z = ReadFloat32_bit();
    unk.serialNum = ReadUint32_bit();
    unk.actorID = FillRefID();
  }

  globalData.data = std::make_shared<ActorCauses>(actorCauses);
}

void SaveFile_::Reader::FillDetectionManager(GlobalData& globalData)
{
  DetectionManager detectionManager;

  detectionManager.numUnknown = ReadVsval_bit();
  detectionManager.unknowns.resize(detectionManager.numUnknown);

  for (auto& unk : detectionManager.unknowns) {
    unk.unknown1 = FillRefID();
    unk.unknown2 = ReadUint32_bit();
    unk.unknown3 = ReadUint32_bit();
  }

  globalData.data = std::make_shared<DetectionManager>(detectionManager);
}

void SaveFile_::Reader::FillLocationMetaData(GlobalData& globalData)
{
  LocationMetaData locationMetaData;

  locationMetaData.numUnknown = ReadVsval_bit();

  locationMetaData.unknowns.resize(locationMetaData.numUnknown);

  for (auto& unk : locationMetaData.unknowns) {
    unk.unknown1 = FillRefID();
    unk.unknown2 = ReadInt32_bit();
  }

  globalData.data = std::make_shared<LocationMetaData>(locationMetaData);
}

void SaveFile_::Reader::FillQuestStaticData(GlobalData& globalData)
{
  QuestStaticData questStaticData;

  questStaticData.numUnknown0 = ReadUint32_bit();
  questStaticData.unknowns0.resize(questStaticData.numUnknown0);

  for (auto& unk0 : questStaticData.unknowns0) {
    unk0.unknown0 = ReadUint32_bit();
    unk0.unknown1 = ReadFloat32_bit();
    unk0.numQuestDataItems = ReadUint32_bit();

    unk0.questRunData_items.resize(unk0.numQuestDataItems);

    for (auto& qrdItem : unk0.questRunData_items) {

      qrdItem.type = ReadUint32_bit(); /// Unknown variable depends on type
                                       /// (1,2,4 = RefID) (3 = Uint32_t)

      switch (qrdItem.type) {
        case 1:
        case 2:
        case 4:
          qrdItem.unknown = std::make_shared<RefID>(FillRefID());
          break;
        case 3:
          qrdItem.unknown = std::make_shared<uint32_t>(ReadUint32_bit());
          break;
        default:
          assert(0);
      }
    }
  }
  questStaticData.numUnknown1 = ReadUint32_bit();
  questStaticData.unknowns1.resize(questStaticData.numUnknown1);

  for (auto& unk1 : questStaticData.unknowns1) {
    unk1.unknown0 = ReadUint32_bit();
    unk1.unknown1 = ReadFloat32_bit();
    unk1.numQuestDataItems = ReadUint32_bit();

    unk1.questRunData_items.resize(unk1.numQuestDataItems);

    for (auto& qrdItem : unk1.questRunData_items) {

      qrdItem.type = ReadUint32_bit(); /// Unknown variable depends on type
                                       /// (1,2,4 = RefID) (3 = Uint32_t)

      switch (qrdItem.type) {
        case 1:
        case 2:
        case 4:
          qrdItem.unknown = std::make_shared<RefID>(FillRefID());
          break;
        case 3:
          qrdItem.unknown = std::make_shared<uint32_t>(ReadUint32_bit());
          break;
        default:
          assert(0);
      }
    }
  }

  questStaticData.numUnknown2 = ReadUint32_bit();
  questStaticData.unknowns2.resize(questStaticData.numUnknown2);

  for (auto& unk2 : questStaticData.unknowns2)
    unk2 = FillRefID();

  questStaticData.numUnknown3 = ReadUint32_bit();
  questStaticData.unknowns3.resize(questStaticData.numUnknown3);

  for (auto& unk3 : questStaticData.unknowns3)
    unk3 = FillRefID();

  questStaticData.numUnknown4 = ReadUint32_bit();
  questStaticData.unknowns4.resize(questStaticData.numUnknown4);

  for (auto& unk4 : questStaticData.unknowns4)
    unk4 = FillRefID();

  questStaticData.numUnknown5 = ReadVsval_bit();
  questStaticData.unknowns5.resize(questStaticData.numUnknown5);

  for (auto& unk5 : questStaticData.unknowns5) {
    unk5.unknown0 = FillRefID();
    unk5.numUnknown1 = ReadVsval_bit();
    unk5.unknowns1.resize(unk5.numUnknown1);

    for (auto& unk5_1 : unk5.unknowns1) {
      unk5_1.unknown0 = ReadUint32_bit();
      unk5_1.unknown1 = ReadUint32_bit();
    }
  }

  questStaticData.unknown6 = Read8_bit();

  globalData.data = std::make_shared<QuestStaticData>(questStaticData);
}

void SaveFile_::Reader::FillStoryTeller(GlobalData& globalData)
{
  StoryTeller storyTeller;
  storyTeller.flag = Read8_bit();
  globalData.data = std::make_shared<StoryTeller>(storyTeller);
}

void SaveFile_::Reader::FillMagicFavorites(GlobalData& globalData)
{
  MagicFavorites magicFavorites;

  magicFavorites.numFavoritedMagics = ReadVsval_bit();
  magicFavorites.favoritedMagics.resize(magicFavorites.numFavoritedMagics);

  for (auto& fm : magicFavorites.favoritedMagics)
    fm = FillRefID();

  magicFavorites.numMagicHotKeys = ReadVsval_bit();
  magicFavorites.magicHotKeys.resize(magicFavorites.numMagicHotKeys);

  for (auto& mHK : magicFavorites.magicHotKeys)
    mHK = FillRefID();

  globalData.data = std::make_shared<MagicFavorites>(magicFavorites);
}

void SaveFile_::Reader::FillPlayerControls(GlobalData& globalData)
{
  PlayerControls playerControls;

  playerControls.unknown1 = Read8_bit();
  playerControls.unknown2 = Read8_bit();
  playerControls.unknown3 = Read8_bit();
  playerControls.unknown4 = Read16_bit();
  playerControls.unknown5 = Read8_bit();

  globalData.data = std::make_shared<PlayerControls>(playerControls);
}

void SaveFile_::Reader::FillStoryEventManager(GlobalData& globalData)
{
  StoryEventManager storyEventManager; /// TODO
  globalData.data = std::make_shared<StoryEventManager>(storyEventManager);
}

void SaveFile_::Reader::FillIngredientShared(GlobalData& globalData)
{
  IngredientShared ingredientShared;

  ingredientShared.numIngredientsCombined = ReadUint32_bit();
  ingredientShared.ingredientsCombined.resize(
    ingredientShared.numIngredientsCombined);

  for (auto& inredientComb : ingredientShared.ingredientsCombined) {
    inredientComb.ingredient0 = FillRefID();
    inredientComb.ingredient1 = FillRefID();
  }

  globalData.data = std::make_shared<IngredientShared>(ingredientShared);
}

void SaveFile_::Reader::FillMenuControls(GlobalData& globalData)
{
  MenuControls menuControls;

  menuControls.unknown1 = Read8_bit();
  menuControls.unknown2 = Read8_bit();

  globalData.data = std::make_shared<MenuControls>(menuControls);
}

void SaveFile_::Reader::FillMenuTopicManager(GlobalData& globalData)
{
  MenuTopicManager menuTopicManager;

  menuTopicManager.unknown1 = FillRefID();
  menuTopicManager.unknown2 = FillRefID();

  globalData.data = std::make_shared<MenuTopicManager>(menuTopicManager);
}

void SaveFile_::Reader::FillTempEffects(GlobalData& globalData)
{
  TempEffects tempEffects;
  /// TODO
  globalData.data = std::make_shared<TempEffects>(tempEffects);
}

void SaveFile_::Reader::FillAnimObjects(GlobalData& globalData)
{
  AnimObjects animObjects;

  animObjects.numObjects = ReadUint32_bit();
  animObjects.objects.resize(animObjects.numObjects);

  for (auto& object : animObjects.objects) {
    object.achr = FillRefID();
    object.anim = FillRefID();
    object.unknown = Read8_bit();
  }

  globalData.data = std::make_shared<AnimObjects>(animObjects);
}

void SaveFile_::Reader::FillTimer(GlobalData& globalData)
{
  Timer timer;

  timer.unknown1 = ReadUint32_bit();
  timer.unknown2 = ReadUint32_bit();

  globalData.data = std::make_shared<Timer>(timer);
}

void SaveFile_::Reader::FillMain(GlobalData& globalData)
{
  std::vector<uint8_t> items;
  items.resize(globalData.length);

  for (auto& item : items)
    item = Read8_bit();

  globalData.data = std::make_shared<std::vector<uint8_t>>(items);
}

uint8_t SaveFile_::Reader::Read8_bit()
{
  return arrayBytes[currentReadPositionInFile++];
}

uint16_t SaveFile_::Reader::Read16_bit()
{
  uint8_t bufer[2];

  for (auto& byte : bufer)
    byte = Read8_bit();

  uint16_t temp = *reinterpret_cast<uint16_t*>(bufer);

  return temp;
}

float SaveFile_::Reader::ReadFloat32_bit()
{
  uint8_t bufer[4];

  for (auto& byte : bufer)
    byte = Read8_bit();

  float temp = *reinterpret_cast<float*>(bufer);

  return temp;
}

uint32_t SaveFile_::Reader::ReadUint32_bit()
{
  uint8_t bufer[4];

  for (auto& byte : bufer)
    byte = Read8_bit();

  uint32_t temp = *reinterpret_cast<uint32_t*>(bufer);

  return temp;
}

int32_t SaveFile_::Reader::ReadInt32_bit()
{
  uint8_t bufer[4];

  for (auto& byte : bufer)
    byte = Read8_bit();

  int32_t temp = *reinterpret_cast<int32_t*>(bufer);

  return temp;
}

uint64_t SaveFile_::Reader::Read64_bit()
{
  uint8_t bufer[8];

  for (auto& byte : bufer)
    byte = Read8_bit();

  uint64_t temp = *reinterpret_cast<uint64_t*>(bufer);

  return temp;
}

std::string SaveFile_::Reader::ReadString(int Size)
{
  std::string temp = "";

  for (int i = 0; i < Size; i++)
    temp += (char)Read8_bit();

  return temp;
}

uint32_t SaveFile_::Reader::ReadVsval_bit()
{
  uint8_t firstByte = Read8_bit();

  switch (firstByte & VsvalTypes::unknown) {
      /// In First Byte Hidden Info by Type this Variable
    case VsvalTypes::uint8:
      return (uint32_t)(firstByte >> 2);

    case VsvalTypes::uint16:
      return (uint32_t)((firstByte | (Read8_bit() << 8)) >>
                        2); /// Read one additional Byte and Create Uint16_t
                            /// and convert in Uint32_t

    case VsvalTypes::uint32:
      return (uint32_t)((firstByte | (Read8_bit() << 8) | (Read8_bit() << 16) |
                         (Read8_bit() << 24)) >>
                        2); /// Read three additional Byte and Create Uint32_t
    default:
      assert(0);
      return 0;
  }
}
