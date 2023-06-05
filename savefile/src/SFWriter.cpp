#include "savefile/SFWriter.h"

SaveFile_::Writer::Writer(std::shared_ptr<SaveFile> saveStructure)
{
  this->saveStructure = saveStructure;
}

bool SaveFile_::Writer::CreateSaveFile(const std::filesystem::path& p)
{

  writer.open(p, std::ios::binary);

  WriteString(13, saveStructure->magic);
  Write(saveStructure->headerSize);

  uint32_t stepBeforeHeader = this->currentWritePositionInFile;

  Write(saveStructure->header.version);
  Write(saveStructure->header.saveNumber);
  WriteString(saveStructure->header.playerName);
  Write(saveStructure->header.playerLevel);
  WriteString(saveStructure->header.playerLocation);
  WriteString(saveStructure->header.gameDate);
  WriteString(saveStructure->header.playerRaceEditorId);
  Write(saveStructure->header.playerSex);
  Write(saveStructure->header.playerCurExp);
  Write(saveStructure->header.playerLvlUpExp);
  Write(saveStructure->header.filetime);
  Write(saveStructure->header.shotWidth);
  Write(saveStructure->header.shotHeight);

  assert(this->currentWritePositionInFile ==
         saveStructure->headerSize + stepBeforeHeader);

  Write(saveStructure->screenshotData);
  Write(saveStructure->formVersion);
  Write(saveStructure->pluginInfoSize);

  uint32_t stepBeforePluginInfo = this->currentWritePositionInFile;

  Write(saveStructure->pluginInfo.numPlugins);
  Write(saveStructure->pluginInfo.pluginsName);

  assert(this->currentWritePositionInFile ==
         saveStructure->pluginInfoSize + stepBeforePluginInfo);

  Write(saveStructure->fileLocationTable.formIDArrayCountOffset);
  Write(saveStructure->fileLocationTable.unknownTable3Offset);
  Write(saveStructure->fileLocationTable.globalDataTable1Offset);
  Write(saveStructure->fileLocationTable.globalDataTable2Offset);
  Write(saveStructure->fileLocationTable.changeFormsOffset);
  Write(saveStructure->fileLocationTable.globalDataTable3Offset);
  Write(saveStructure->fileLocationTable.globalDataTable1Count);
  Write(saveStructure->fileLocationTable.globalDataTable2Count);
  Write(saveStructure->fileLocationTable.globalDataTable3Count);
  Write(saveStructure->fileLocationTable.changeFormCount);
  Write(saveStructure->fileLocationTable.unused);

  assert(this->currentWritePositionInFile ==
         saveStructure->fileLocationTable.globalDataTable1Offset);
  for (auto& item : saveStructure->globalDataTable1)
    WriteGlobalDataTable(item);

  assert(this->currentWritePositionInFile ==
         saveStructure->fileLocationTable.globalDataTable2Offset);
  for (auto& item : saveStructure->globalDataTable2)
    WriteGlobalDataTable(item);

  assert(this->currentWritePositionInFile ==
         saveStructure->fileLocationTable.changeFormsOffset);
  for (auto& form : saveStructure->changeForms) {
    WriteRefID(form.formID);
    Write(form.changeFlags);
    Write(form.type);
    Write(form.version);

    if (form.type > 0x3F && form.type <= 0x7F) {
      Write((uint16_t)form.length1);
      Write((uint16_t)form.length2);
    } else if (form.type > 0x7F) {
      Write(form.length1);
      Write(form.length2);
    } else {
      Write((uint8_t)form.length1);
      Write((uint8_t)form.length2);
    }

    Write(form.data);
  }

  assert(this->currentWritePositionInFile ==
         saveStructure->fileLocationTable.globalDataTable3Offset);
  for (auto& item : saveStructure->globalDataTable3)
    WriteGlobalDataTable(item);

  Write(saveStructure
          ->fixForBag); /// fix for next assert , globalDataTable3Count is
                        /// currently bugged (as of version 112) that it does
                        /// not include type 1001 (Papyrus) in the count.

  assert(this->currentWritePositionInFile ==
         saveStructure->fileLocationTable.formIDArrayCountOffset);
  Write(saveStructure->formIDArrayCount);
  Write(saveStructure->formIDArray);

  Write(saveStructure->visitedWorldspaceArrayCount);
  Write(saveStructure->visitedWorldspaceArray);

  assert(this->currentWritePositionInFile ==
         saveStructure->fileLocationTable.unknownTable3Offset);
  Write(saveStructure->unknown3TableSize);

  uint32_t stepBeforeUnknownTable = this->currentWritePositionInFile;

  Write(saveStructure->unknown3Table.count);
  Write(saveStructure->unknown3Table.unknown);

  assert(this->currentWritePositionInFile ==
         saveStructure->unknown3TableSize + stepBeforeUnknownTable);

  writer.close();

  return !writer.fail();
}

void SaveFile_::Writer::WriteGlobalDataTable(GlobalData& globalData)
{
  Write(globalData.type);
  Write(globalData.length);

  uint32_t stepAfterWriteData =
    globalData.length + this->currentWritePositionInFile;

  switch (globalData.type) {

    case 0:
      WriteMiscStats(static_cast<MiscStats*>(globalData.data.get()));
      break;
    case 1:
      WritePlayerLocation(static_cast<PlayerLocation*>(globalData.data.get()));
      break;
    case 2:
      WriteTES(static_cast<TES*>(globalData.data.get()));
      break;
    case 3:
      WriteGlobalVariables(
        static_cast<GlobalVariables*>(globalData.data.get()));
      break;
    case 4:
      WriteCreatedObjects(static_cast<CreatedObjects*>(globalData.data.get()));
      break;
    case 5:
      WriteEffects(static_cast<Effects*>(globalData.data.get()));
      break;
    case 6:
      WriteWeather(static_cast<Weather*>(globalData.data.get()));
      break;
    case 7:
      WriteAudio(static_cast<Audio*>(globalData.data.get()));
      break;
    case 8:
      WriteSkyCells(static_cast<SkyCells*>(globalData.data.get()));
      break;
    case 100:
      WriteProcessList(static_cast<ProcessList*>(globalData.data.get()));
      break;
    case 101:
      WriteCombat(static_cast<Combat*>(globalData.data.get()));
      break;
    case 102:
      WriteInterface(static_cast<Interface*>(globalData.data.get()));
      break;
    case 103:
      WriteActorCauses(static_cast<ActorCauses*>(globalData.data.get()));
      break;
    case 104:
      WriteMain(static_cast<std::vector<uint8_t>*>(
        globalData.data.get())); /// TODO Unknown format.
      break;
    case 105:
      WriteDetectionManager(
        static_cast<DetectionManager*>(globalData.data.get()));
      break;
    case 106:
      WriteLocationMetaData(
        static_cast<LocationMetaData*>(globalData.data.get()));
      break;
    case 107:
      WriteQuestStaticData(
        static_cast<QuestStaticData*>(globalData.data.get()));
      break;
    case 108:
      WriteStoryTeller(static_cast<StoryTeller*>(globalData.data.get()));
      break;
    case 109:
      WriteMagicFavorites(static_cast<MagicFavorites*>(globalData.data.get()));
      break;
    case 110:
      WritePlayerControls(static_cast<PlayerControls*>(globalData.data.get()));
      break;
    case 111:
      WriteMain(static_cast<std::vector<uint8_t>*>(
        globalData.data.get())); /// TODO Unknown format.
      break;
    case 112:
      WriteIngredientShared(
        static_cast<IngredientShared*>(globalData.data.get()));
      break;
    case 113:
      WriteMenuControls(static_cast<MenuControls*>(globalData.data.get()));
      break;
    case 114:
      WriteMenuTopicManager(
        static_cast<MenuTopicManager*>(globalData.data.get()));
      break;
    case 1000:
      WriteMain(
        static_cast<std::vector<uint8_t>*>(globalData.data.get())); /// TODO
      break;
    case 1001:
      WriteMain(
        static_cast<std::vector<uint8_t>*>(globalData.data.get())); /// TODO
      break;
    case 1002:
      WriteAnimObjects(static_cast<AnimObjects*>(globalData.data.get()));
      break;
    case 1003:
      WriteTimer(static_cast<Timer*>(globalData.data.get()));
      break;
    case 1004:
      WriteMain(static_cast<std::vector<uint8_t>*>(
        globalData.data.get())); /// TODO Unknown format.
      break;
    case 1005:
      WriteMain(static_cast<std::vector<uint8_t>*>(
        globalData.data.get())); /// TODO Always Empty.
      break;
    default:
      assert(0);
      break;
  }
  assert(stepAfterWriteData == this->currentWritePositionInFile);
}

void SaveFile_::Writer::WriteMiscStats(MiscStats* miscStats)
{
  Write(miscStats->numStats);

  for (auto& stat : miscStats->stats) {
    WriteString(stat.name);
    Write(stat.category);
    Write(stat.value);
  }
}

void SaveFile_::Writer::WritePlayerLocation(PlayerLocation* playerLocation)
{
  Write(playerLocation->nextObjectId);
  WriteRefID(playerLocation->worldspace1);
  Write(playerLocation->coorX);
  Write(playerLocation->coorY);
  WriteRefID(playerLocation->worldspace2);
  Write(playerLocation->posX);
  Write(playerLocation->posY);
  Write(playerLocation->posZ);
  Write(playerLocation->unknown);
}

void SaveFile_::Writer::WriteTES(TES* tes)
{
  WriteVsval(tes->numUnknown1);
  for (auto& unk0 : tes->unknowns1) {
    WriteRefID(unk0.formID);
    Write(unk0.unknown);
  }

  Write(tes->numUnknown2);
  for (auto& refID2 : tes->unknowns2)
    WriteRefID(refID2);

  WriteVsval(tes->numUnknown3);
  for (auto& refID3 : tes->unknowns3)
    WriteRefID(refID3);
}

void SaveFile_::Writer::WriteGlobalVariables(GlobalVariables* globalVariables)
{
  WriteVsval(globalVariables->numGlobals);
  for (auto& global : globalVariables->globals) {
    WriteRefID(global.formID);
    Write(global.value);
  }
}

void SaveFile_::Writer::WriteCreatedObjects(CreatedObjects* createdObjects)
{
  WriteVsval(createdObjects->numWeapon);
  for (auto& weapon : createdObjects->weaponEnchTable)
    WriteEnchantment(weapon);

  WriteVsval(createdObjects->numArmor);
  for (auto& armour : createdObjects->armourEnchTable)
    WriteEnchantment(armour);

  WriteVsval(createdObjects->numPotion);
  for (auto& potion : createdObjects->potionTable)
    WriteEnchantment(potion);

  WriteVsval(createdObjects->numPoison);
  for (auto& poison : createdObjects->poisonTable)
    WriteEnchantment(poison);
}

void SaveFile_::Writer::WriteEnchantment(
  CreatedObjects::Enchantment& enchantment)
{
  WriteRefID(enchantment.refID);
  Write(enchantment.timesUsed);
  WriteVsval(enchantment.numEffects);
  for (auto& effect : enchantment.effects)
    WriteMagicEffect(effect);
}

void SaveFile_::Writer::WriteMagicEffect(
  CreatedObjects::Enchantment::MagicEffect& magicEffect)
{
  WriteRefID(magicEffect.effectID);
  Write(magicEffect.info.magnitude);
  Write(magicEffect.info.duration);
  Write(magicEffect.info.area);
  Write(magicEffect.price);
}

void SaveFile_::Writer::WriteEffects(Effects* effects)
{
  WriteVsval(effects->numImgSpaceMod);
  for (auto& imgSM : effects->imageSpaceModifiers) {
    Write(imgSM.strength);
    Write(imgSM.timestamp);
    Write(imgSM.unknown);
    WriteRefID(imgSM.effectID);
  }
  Write(effects->unknown1);
  Write(effects->unknown2);
}

void SaveFile_::Writer::WriteWeather(Weather* weather)
{
  WriteRefID(weather->climate);
  WriteRefID(weather->weather);
  WriteRefID(weather->prevWeather);
  WriteRefID(weather->unknownWeather1);
  WriteRefID(weather->unknownWeather2);
  WriteRefID(weather->regnWeather);
  Write(weather->curTime);
  Write(weather->begTime);
  Write(weather->weatherPct);
  Write(weather->unknown1);
  Write(weather->unknown2);
  Write(weather->unknown3);
  Write(weather->unknown4);
  Write(weather->unknown5);
  Write(weather->unknown6);
  Write(weather->unknown7);
  Write(weather->unknown8);
  Write(weather->flags);

  assert((weather->flags | 0b11111110) != 0b11111111);
  // weather.unknown9 = Only present if (weather.flags | 0b11111110) ==
  // 0b11111111)

  assert((weather->flags | 0b11111101) != 0b11111111);
  // weather.unknown10 = Only present if (weather.flags | 0b11111101) ==
  // 0b11111111)
}

void SaveFile_::Writer::WriteAudio(Audio* audio)
{
  WriteRefID(audio->unknown);
  WriteVsval(audio->numTracks);
  for (auto& track : audio->tracks)
    WriteRefID(track);
  WriteRefID(audio->bgm);
}

void SaveFile_::Writer::WriteSkyCells(SkyCells* skyCells)
{
  WriteVsval(skyCells->numUnknown);
  for (auto& unk : skyCells->unknowns) {
    WriteRefID(unk.unknown1);
    WriteRefID(unk.unknown2);
  }
}

void SaveFile_::Writer::WriteProcessList(ProcessList* processList)
{
  Write(processList->unknown1);
  Write(processList->unknown2);
  Write(processList->unknown3);
  Write(processList->numNext);
  for (auto& crimeT : processList->allCrimeTypes)
    WriteCrimeType(crimeT);
}

void SaveFile_::Writer::WriteCrimeType(ProcessList::CrimeType& crimeType)
{
  WriteVsval(crimeType.numCrimes);
  for (auto& crime : crimeType.crimes) {
    Write(crime.witnessNum);
    Write(crime.crimeType);
    Write(crime.unknown1);
    Write(crime.quantity);
    Write(crime.numSerial);
    Write(crime.unknown2);
    Write(crime.unknown3);
    Write(crime.elapsedTime);

    WriteRefID(crime.victimID);
    WriteRefID(crime.criminalID);
    WriteRefID(crime.itemBaseID);
    WriteRefID(crime.ownershipID);

    WriteVsval(crime.numWitnesses);

    for (auto& witnes : crime.witnesses)
      WriteRefID(witnes);

    Write(crime.bounty);
    WriteRefID(crime.crimeFactionID);
    Write(crime.isCleared);
    Write(crime.unknown4);
  }
}

void SaveFile_::Writer::WriteCombat(Combat* combat)
{
  Write(combat->nextNum);
  WriteVsval(combat->numUnknown0);
  for (auto& unk0 : combat->unknowns0) {
    Write(unk0.unknown1);
    Write(unk0.serialNum);
    WriteCombat_Unknown0_0(unk0.unknown2);
  }

  WriteVsval(combat->numUnknown1);
  for (auto& unk1 : combat->unknowns1) {
    WriteRefID(unk1.unknown1);
    Write(unk1.unknown2);
    WriteRefID(unk1.unknown3);
    WriteRefID(unk1.unknown4);
    Write(unk1.unknown5);
    Write(unk1.unknown6);
    Write(unk1.unknown7);
    Write(unk1.x);
    Write(unk1.y);
    Write(unk1.z);
    Write(unk1.unknown8);
    Write(unk1.unknown9);
    Write(unk1.unknown10);
    Write(unk1.unknown11);
    Write(unk1.unknown12);
    Write(unk1.unknown13);
    Write(unk1.unknown14);
    Write(unk1.unknown15);
    WriteRefID(unk1.unknown16);
  }

  Write(combat->unknown2);
  WriteVsval(combat->unknown3);
  WriteVsval(combat->numUnknown4);
  for (auto& refID : combat->unknowns4)
    WriteRefID(refID);

  Write(combat->unknown5);

  Write(combat->unknown6.unknown1);
  Write(combat->unknown6.unknown2);

  Write(combat->unknown7.unknown1);
  Write(combat->unknown7.unknown2);
}

void SaveFile_::Writer::WriteCombat_Unknown0_0(Combat::Unknown0_0& unknown0_0)
{
  WriteVsval(unknown0_0.numUnknown0);
  for (auto& unk0_0_0 : unknown0_0.unknowns0)
    WriteCombat_Unknown0_0_0(unk0_0_0);

  WriteVsval(unknown0_0.numUnknown1);
  for (auto& unk0_0_1 : unknown0_0.unknowns1) {

    WriteRefID(unk0_0_1.unknown1);
    Write(unk0_0_1.unknown2);
    Write(unk0_0_1.unknown3);
  }

  Write(unknown0_0.unknown2.unknown1);
  Write(unknown0_0.unknown2.unknown2);

  Write(unknown0_0.unknown3.unknown1);
  Write(unknown0_0.unknown3.unknown2);

  Write(unknown0_0.unknown4.unknown1);
  Write(unknown0_0.unknown4.unknown2);

  Write(unknown0_0.unknown5.unknown1);
  Write(unknown0_0.unknown5.unknown2);

  for (auto& unk0_0_6 : unknown0_0.unknowns6) {
    Write(unk0_0_6.unknown1);
    Write(unk0_0_6.unknown2);
  }

  Write(unknown0_0.unknownFlag);

  if (unknown0_0.unknownFlag != 0)
    WriteCombat_Unknown0_0_2(unknown0_0.unknown7);

  Write(unknown0_0.unknown8.unknown1);
  Write(unknown0_0.unknown8.unknown2);

  Write(unknown0_0.unknown9);
  Write(unknown0_0.unknown10);
  Write(unknown0_0.unknown11);
  Write(unknown0_0.unknown12);

  Write(unknown0_0.unknown13.unknown1);
  Write(unknown0_0.unknown13.unknown2);

  Write(unknown0_0.unknown14);
}

void SaveFile_::Writer::WriteCombat_Unknown0_0_0(
  Combat::Unknown0_0::Unknown0_0_0& unknown0_0_0)
{
  WriteRefID(unknown0_0_0.unknown1);

  Write(unknown0_0_0.unknown2);
  Write(unknown0_0_0.unknown3);
  Write(unknown0_0_0.unknown4);
  Write(unknown0_0_0.unknown5);

  WriteRefID(unknown0_0_0.target);

  Write(unknown0_0_0.unknown6.x);
  Write(unknown0_0_0.unknown6.y);
  Write(unknown0_0_0.unknown6.z);
  WriteRefID(unknown0_0_0.unknown6.cellID);

  Write(unknown0_0_0.unknown7.x);
  Write(unknown0_0_0.unknown7.y);
  Write(unknown0_0_0.unknown7.z);
  WriteRefID(unknown0_0_0.unknown7.cellID);

  Write(unknown0_0_0.unknown8.x);
  Write(unknown0_0_0.unknown8.y);
  Write(unknown0_0_0.unknown8.z);
  WriteRefID(unknown0_0_0.unknown8.cellID);

  Write(unknown0_0_0.unknown9.x);
  Write(unknown0_0_0.unknown9.y);
  Write(unknown0_0_0.unknown9.z);
  WriteRefID(unknown0_0_0.unknown9.cellID);

  Write(unknown0_0_0.unknown10.x);
  Write(unknown0_0_0.unknown10.y);
  Write(unknown0_0_0.unknown10.z);
  WriteRefID(unknown0_0_0.unknown10.cellID);

  Write(unknown0_0_0.unknown11);
  Write(unknown0_0_0.unknown12);
  Write(unknown0_0_0.unknown13);
  Write(unknown0_0_0.unknown14);
  Write(unknown0_0_0.unknown15);
  Write(unknown0_0_0.unknown16);
}

void SaveFile_::Writer::WriteCombat_Unknown0_0_2(
  Combat::Unknown0_0::Unknown0_0_2& unknown0_0_2)
{
  WriteRefID(unknown0_0_2.unknown1);

  Write(unknown0_0_2.unknown2.unknown1);
  Write(unknown0_0_2.unknown2.unknown2);

  Write(unknown0_0_2.unknown3.unknown1);
  Write(unknown0_0_2.unknown3.unknown2);

  Write(unknown0_0_2.unknown4);

  Write(unknown0_0_2.unknown5.x);
  Write(unknown0_0_2.unknown5.y);
  Write(unknown0_0_2.unknown5.z);
  WriteRefID(unknown0_0_2.unknown5.cellID);

  Write(unknown0_0_2.unknown6);

  WriteVsval(unknown0_0_2.numUnknown7);
  for (auto& unk0_0_2_7 : unknown0_0_2.unknowns7) {

    Write(unk0_0_2_7.unknown1.x);
    Write(unk0_0_2_7.unknown1.y);
    Write(unk0_0_2_7.unknown1.z);
    WriteRefID(unk0_0_2_7.unknown1.cellID);

    Write(unk0_0_2_7.unknown2);
    Write(unk0_0_2_7.unknown3);
  }

  WriteVsval(unknown0_0_2.numUnknown8);
  for (auto& unk0_0_2_8 : unknown0_0_2.unknowns8) {

    WriteRefID(unk0_0_2_8.unknown1);
    WriteRefID(unk0_0_2_8.unknown2);

    Write(unk0_0_2_8.unknown3);
    Write(unk0_0_2_8.unknown4);
    Write(unk0_0_2_8.unknown5);
    Write(unk0_0_2_8.unknown6);
  }

  Write(unknown0_0_2.unknownFlag);

  WriteCombat_Unknown0_0_2_2(unknown0_0_2.unknown9);
}

void SaveFile_::Writer::WriteCombat_Unknown0_0_2_2(
  Combat::Unknown0_0::Unknown0_0_2::Unknown0_0_2_2& unknown0_0_2_2)
{
  Write(unknown0_0_2_2.unknown1);
  Write(unknown0_0_2_2.unknown2);
  Write(unknown0_0_2_2.numUnknown3);

  for (auto& unk0_0_2_2_3 : unknown0_0_2_2.unknowns3) {
    Write(unk0_0_2_2_3.unknown1);
    Write(unk0_0_2_2_3.numUnknown2);
    Write(unk0_0_2_2_3.unknowns2);
    WriteRefID(unk0_0_2_2_3.unknown3);
    Write(unk0_0_2_2_3.unknown4);
  }

  WriteRefID(unknown0_0_2_2.unknown4);

  Write(unknown0_0_2_2.unknown5);
  Write(unknown0_0_2_2.unknown6);
  Write(unknown0_0_2_2.unknown7);
  Write(unknown0_0_2_2.unknown8);
  Write(unknown0_0_2_2.unknown9);

  WriteRefID(unknown0_0_2_2.unknown10);
  Write(unknown0_0_2_2.unknown11);
  WriteRefID(unknown0_0_2_2.unknown12);

  Write(unknown0_0_2_2.unknown13);
  Write(unknown0_0_2_2.unknown14);
  Write(unknown0_0_2_2.unknown15);
  Write(unknown0_0_2_2.unknown16);
  Write(unknown0_0_2_2.unknown17);
}

void SaveFile_::Writer::WriteInterface(Interface* _interface)
{
  Write(_interface->numShownHelpMsg);
  Write(_interface->shownHelpMsg);
  Write(_interface->unknown1);

  WriteVsval(_interface->numLastUsedWeapons);
  for (auto& refID : _interface->lastUsedWeapons)
    WriteRefID(refID);

  WriteVsval(_interface->numLastUsedSpells);
  for (auto& refID : _interface->lastUsedSpells)
    WriteRefID(refID);

  WriteVsval(_interface->numLastUsedShouts);
  for (auto& refID : _interface->lastUsedShouts)
    WriteRefID(refID);

  Write(_interface->unknown2);

  WriteInterface_Unknown0(_interface->unknown3);
}

void SaveFile_::Writer::WriteInterface_Unknown0(Interface::Unknown0& unknown0)
{
  WriteVsval(unknown0.numUnknown1);
  for (auto& unk0_1 : unknown0.unknowns1) {

    WriteString(unk0_1.unknown1);
    WriteString(unk0_1.unknown2);

    Write(unk0_1.unknown3);
    Write(unk0_1.unknown4);
    Write(unk0_1.unknown5);
    Write(unk0_1.unknown6);
  }

  WriteVsval(unknown0.numUnknown2);
  for (auto& unk0_2 : unknown0.unknowns2)
    WriteString(unk0_2);

  Write(unknown0.unknown3);
}

void SaveFile_::Writer::WriteActorCauses(ActorCauses* actorCauses)
{
  Write(actorCauses->nextNum);
  WriteVsval(actorCauses->numUnknown);
  for (auto& unk : actorCauses->unknowns) {
    Write(unk.x);
    Write(unk.y);
    Write(unk.z);
    Write(unk.serialNum);
    WriteRefID(unk.actorID);
  }
}

void SaveFile_::Writer::WriteDetectionManager(
  DetectionManager* detectionManager)
{
  WriteVsval(detectionManager->numUnknown);
  for (auto& unk : detectionManager->unknowns) {
    WriteRefID(unk.unknown1);
    Write(unk.unknown2);
    Write(unk.unknown3);
  }
}

void SaveFile_::Writer::WriteLocationMetaData(
  LocationMetaData* locationMetaData)
{
  WriteVsval(locationMetaData->numUnknown);
  for (auto& unk : locationMetaData->unknowns) {
    WriteRefID(unk.unknown1);
    Write(unk.unknown2);
  }
}

void SaveFile_::Writer::WriteQuestStaticData(QuestStaticData* questStaticData)
{
  Write(questStaticData->numUnknown0);
  for (auto& questRD3_0 : questStaticData->unknowns0)
    WriteQuestRunData_3(questRD3_0);

  Write(questStaticData->numUnknown1);
  for (auto& questRD3_1 : questStaticData->unknowns1)
    WriteQuestRunData_3(questRD3_1);

  Write(questStaticData->numUnknown2);
  for (auto& refID2 : questStaticData->unknowns2)
    WriteRefID(refID2);

  Write(questStaticData->numUnknown3);
  for (auto& refID3 : questStaticData->unknowns3)
    WriteRefID(refID3);

  Write(questStaticData->numUnknown4);
  for (auto& refID4 : questStaticData->unknowns4)
    WriteRefID(refID4);

  WriteVsval(questStaticData->numUnknown5);
  for (auto& unk5 : questStaticData->unknowns5) {

    WriteRefID(unk5.unknown0);
    WriteVsval(unk5.numUnknown1);

    for (auto& unk5_1 : unk5.unknowns1) {
      Write(unk5_1.unknown0);
      Write(unk5_1.unknown1);
    }
  }

  Write(questStaticData->unknown6);
}

void SaveFile_::Writer::WriteQuestRunData_3(
  QuestStaticData::QuestRunData_3& questRunData_3)
{
  Write(questRunData_3.unknown0);
  Write(questRunData_3.unknown1);
  Write(questRunData_3.numQuestDataItems);
  for (auto& item : questRunData_3.questRunData_items) {
    Write(item.type);

    switch (item.type) { /// Unknown variable depends on type (1,2,4 = RefID)
                         /// (3 = Uint32_t)
      case 1:
      case 2:
      case 4:
        WriteRefID(*(static_cast<RefID*>(item.unknown.get())));
        break;
      case 3:
        Write(*(static_cast<uint32_t*>(item.unknown.get())));
        break;
      default:
        assert(0);
    }
  }
}

void SaveFile_::Writer::WriteStoryTeller(StoryTeller* storyTeller)
{
  Write(storyTeller->flag);
}

void SaveFile_::Writer::WriteMagicFavorites(MagicFavorites* magicFavorites)
{
  WriteVsval(magicFavorites->numFavoritedMagics);
  for (auto& refID : magicFavorites->favoritedMagics)
    WriteRefID(refID);

  WriteVsval(magicFavorites->numMagicHotKeys);
  for (auto& refID : magicFavorites->magicHotKeys)
    WriteRefID(refID);
}

void SaveFile_::Writer::WritePlayerControls(PlayerControls* playerControls)
{
  Write(playerControls->unknown1);
  Write(playerControls->unknown2);
  Write(playerControls->unknown3);
  Write(playerControls->unknown4);
  Write(playerControls->unknown5);
}

void SaveFile_::Writer::WriteStoryEventManager(
  StoryEventManager* storyEventManager)
{
  /// TODO

  Write(storyEventManager->unknown0);
  WriteVsval(storyEventManager->count);
  Write(storyEventManager->data);
}

void SaveFile_::Writer::WriteIngredientShared(
  IngredientShared* ingredientShared)
{
  Write(ingredientShared->numIngredientsCombined);
  for (auto& ingrComb : ingredientShared->ingredientsCombined) {
    WriteRefID(ingrComb.ingredient0);
    WriteRefID(ingrComb.ingredient1);
  }
}

void SaveFile_::Writer::WriteMenuControls(MenuControls* menuControls)
{

  Write(menuControls->unknown1);
  Write(menuControls->unknown2);
}

void SaveFile_::Writer::WriteMenuTopicManager(
  MenuTopicManager* menuTopicManager)
{
  WriteRefID(menuTopicManager->unknown1);
  WriteRefID(menuTopicManager->unknown2);
}

void SaveFile_::Writer::WriteTempEffects(TempEffects* tempEffects)
{
}

void SaveFile_::Writer::WriteAnimObjects(AnimObjects* animObjects)
{
  Write(animObjects->numObjects);
  for (auto& obj : animObjects->objects) {
    WriteRefID(obj.achr);
    WriteRefID(obj.anim);
    Write(obj.unknown);
  }
}

void SaveFile_::Writer::WriteTimer(Timer* timer)
{
  Write(timer->unknown1);
  Write(timer->unknown2);
}

void SaveFile_::Writer::WriteMain(std::vector<uint8_t>* data)
{
  Write(*data);
}

void SaveFile_::Writer::WriteString(const std::string& str)
{
  uint16_t length = (uint16_t)str.size();
  Write(length);

  for (int i = 0; i < length; ++i)
    this->writer.put(str.at(i));
  currentWritePositionInFile = currentWritePositionInFile + length;
}

void SaveFile_::Writer::WriteString(int length, std::string& str)
{
  for (int i = 0; i < length; ++i)
    this->writer.put(str.at(i));
  currentWritePositionInFile = currentWritePositionInFile + length;
}

void SaveFile_::Writer::WriteRefID(RefID& refID)
{
  Write(refID.byte0);
  Write(refID.byte1);
  Write(refID.byte2);
}

void SaveFile_::Writer::WriteVsval(uint32_t& vsval)
{
  if (vsval <= 0x3F) { // value is uint8 _0x3f
    Write(static_cast<uint8_t>(vsval << 2));
  } else if (vsval <= 0x3FFF) { // value is uint16
    Write(static_cast<uint16_t>((vsval << 2) | 1));
  } else { // value is uint32
    Write((vsval << 2) | 2);
  }
}
