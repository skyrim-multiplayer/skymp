#pragma once
#include "SFStructure.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

namespace SaveFile_ {
class Writer
{
public:
  Writer(std::shared_ptr<SaveFile> saveStructure);
  bool CreateSaveFile(const std::filesystem::path& path);

private:
  std::ofstream writer;
  std::ifstream ifstr;
  std::shared_ptr<SaveFile> saveStructure;

  uint32_t currentWritePositionInFile = 0;

  template <class T>
  void Write(const T& data)
  {
    writer.write((char*)&data, sizeof(data));
    currentWritePositionInFile = currentWritePositionInFile + sizeof(data);
  };

  template <class T>
  void Write(const std::vector<T>& vector)
  {
    for (auto& item : vector)
      Writer::Write(item);
  };

  void Write(const std::vector<std::string>& vector)
  {
    for (auto& item : vector)
      WriteString(item);
  };

  void WriteMiscStats(MiscStats* miscStats);
  void WritePlayerLocation(PlayerLocation* playerLocation);
  void WriteTES(TES* tes);
  void WriteGlobalVariables(GlobalVariables* globalVariables);
  void WriteCreatedObjects(CreatedObjects* createdObjects);

  void WriteEnchantment(CreatedObjects::Enchantment& enchantment);
  void WriteMagicEffect(CreatedObjects::Enchantment::MagicEffect& magicEffect);

  void WriteEffects(Effects* effects);
  void WriteWeather(Weather* weather);
  void WriteAudio(Audio* audio);
  void WriteSkyCells(SkyCells* skyCells);

  void WriteProcessList(ProcessList* processList);
  void WriteCrimeType(ProcessList::CrimeType& crimeType);
  void WriteCombat(Combat* combat);
  void WriteCombat_Unknown0_0(Combat::Unknown0_0& unknown0_0);
  void WriteCombat_Unknown0_0_0(
    Combat::Unknown0_0::Unknown0_0_0& unknown0_0_0);
  void WriteCombat_Unknown0_0_2(
    Combat::Unknown0_0::Unknown0_0_2& unknown0_0_2);
  void WriteCombat_Unknown0_0_2_2(
    Combat::Unknown0_0::Unknown0_0_2::Unknown0_0_2_2& unknown0_0_2_2);

  void WriteInterface(Interface* _interface);
  void WriteInterface_Unknown0(Interface::Unknown0& unknown0);

  void WriteActorCauses(ActorCauses* actorCauses);
  // Unknown 104
  void WriteDetectionManager(DetectionManager* detectionManager);
  void WriteLocationMetaData(LocationMetaData* locationMetaData);
  void WriteQuestStaticData(QuestStaticData* questStaticData);
  void WriteQuestRunData_3(QuestStaticData::QuestRunData_3& questRunData_3);
  void WriteStoryTeller(StoryTeller* storyTeller);
  void WriteMagicFavorites(MagicFavorites* magicFavorites);
  void WritePlayerControls(PlayerControls* playerControls);
  void WriteStoryEventManager(StoryEventManager* storyEventManager);
  void WriteIngredientShared(IngredientShared* ingredientShared);
  void WriteMenuControls(MenuControls* menuControls);
  void WriteMenuTopicManager(MenuTopicManager* menuTopicManager);

  void WriteTempEffects(TempEffects* tempEffects);
  // Papyrus
  void WriteAnimObjects(AnimObjects* animObjects);
  void WriteTimer(Timer* timer);
  // Synchronized Animations
  void WriteMain(std::vector<uint8_t>* data);

  void WriteString(const std::string& str);
  void WriteString(int length, std::string& str);

  void WriteRefID(RefID& refID);
  void WriteVsval(uint32_t& vsval);
  void WriteGlobalDataTable(GlobalData& globalData);
};
}
