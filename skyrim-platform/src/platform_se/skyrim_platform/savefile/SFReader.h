#pragma once
#include "SFStructure.h"

namespace SaveFile_ {
class Reader
{
public:
  std::shared_ptr<SaveFile> GetStructure() { return this->structure; };
  Reader(std::string path);
  Reader(const uint8_t* data, size_t size);

private:
  std::string path = "";

  int currentReadPositionInFile = 0;

  std::vector<uint8_t> arrayBytes;

  std::shared_ptr<SaveFile> structure;

  Header FillHeader();
  PluginInfo FillPluginInfo();
  FileLocationTable FillFileLocationTable();
  std::vector<GlobalData> FillGlobalData(uint32_t numObject);
  std::vector<ChangeForm> FillChangeForm(uint32_t numObject);
  RefID FillRefID();
  Unknown3Table FillUnknown3Table();

  void FillMiscStats(GlobalData& globalData);
  void FillPlayerLocation(GlobalData& globalData);
  void FillTES(GlobalData& globalData);
  void FillGlobalVariables(GlobalData& globalData);
  void FillCreatedObjects(GlobalData& globalData);
  void FillEffects(GlobalData& globalData);
  void FillWeather(GlobalData& globalData);
  void FillAudio(GlobalData& globalData);
  void FillSkyCells(GlobalData& globalData);

  void FillProcessList(GlobalData& globalData);
  void FillCombat(GlobalData& globalData);
  void FillInterface(GlobalData& globalData);
  void FillActorCauses(GlobalData& globalData);
  // Unknown 104
  void FillDetectionManager(GlobalData& globalData);
  void FillLocationMetaData(GlobalData& globalData);
  void FillQuestStaticData(GlobalData& globalData);
  void FillStoryTeller(GlobalData& globalData);
  void FillMagicFavorites(GlobalData& globalData);
  void FillPlayerControls(GlobalData& globalData);
  void FillStoryEventManager(GlobalData& globalData);
  void FillIngredientShared(GlobalData& globalData);
  void FillMenuControls(GlobalData& globalData);
  void FillMenuTopicManager(GlobalData& globalData);

  void FillTempEffects(GlobalData& globalData);
  // Papyrus
  void FillAnimObjects(GlobalData& globalData);
  void FillTimer(GlobalData& globalData);
  // Synchronized Animations
  void FillMain(GlobalData& globalData);

  // void Fill(GlobalData& globalData);

  void FillUnknown0_0_2(const uint32_t& flag, Combat::Unknown0& unk0);

  uint8_t Read8_bit();
  uint16_t Read16_bit();
  uint32_t ReadUint32_bit();
  int32_t ReadInt32_bit();
  float ReadFloat32_bit();
  uint64_t Read64_bit();
  std::string ReadString(int Size);

  enum VsvalTypes
  {
    uint8 = 0b00000000,
    uint16 = 0b00000001,
    uint32 = 0b00000010,
    unknown = 0b00000011
  };
  uint32_t ReadVsval_bit();

  void Read();
  void CreateScriptStructure(std::vector<uint8_t> arrayBytes);
};
}
