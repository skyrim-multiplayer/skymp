#include "LoadGameApi.h"
#include "JsExtractPoint.h"
#include "LoadGame.h"
#include "NullPointerException.h"
#include "savefile/SFChangeFormNPC.h"

namespace {
uint32_t RgbToAbgr(int32_t rgb)
{
  uint32_t& colorUint = reinterpret_cast<uint32_t&>(rgb);

  colorUint *= 256; // RGB => RGBA

  uint8_t rgba[4];
  for (int i = 0; i < std::size(rgba); ++i) {
    rgba[i] = colorUint / (int)pow(256, std::size(rgba) - i - 1);
    colorUint %= (int)pow(256, std::size(rgba) - i - 1);
  }

  uint32_t resultColor = 0;
  for (int i = 0; i < std::size(rgba); ++i) {
    resultColor +=
      rgba[std::size(rgba) - i - 1] * (int)pow(256, std::size(rgba) - i - 1);
  }
  return resultColor;
}

SaveFile_::RefID FormIdToRefId(uint32_t formId)
{
  // Note: Tested only with normal Skyrim.esm ids

  std::string binType = formId >= 0x01000000 ? "02" : "01";
  std::string binId = std::bitset<22>(formId).to_string();

  std::string binSum = binType + binId;
  std::string binByte0 = { binSum.begin(), binSum.begin() + 8 };
  std::string binByte1 = { binSum.begin() + 8, binSum.begin() + 16 };
  std::string binByte2 = { binSum.begin() + 16, binSum.begin() + 24 };

  SaveFile_::RefID hpRefId;
  hpRefId.byte0 = std::bitset<8>(binByte0).to_ulong();
  hpRefId.byte1 = std::bitset<8>(binByte1).to_ulong();
  hpRefId.byte2 = std::bitset<8>(binByte2).to_ulong();
  return hpRefId;
}

std::unique_ptr<SaveFile_::ChangeFormNPC_> CreateChangeFormNpc(
  std::shared_ptr<SaveFile_::SaveFile> save, JsValue npcData)
{
  if (npcData.GetType() != JsType::Object)
    return nullptr;

  std::unique_ptr<SaveFile_::ChangeFormNPC_> changeFormNpc;
  changeFormNpc.reset(new SaveFile_::ChangeFormNPC_);

  if (auto name = npcData.GetProperty("name");
      name.GetType() == JsType::String)
    changeFormNpc->playerName = static_cast<std::string>(name);

  if (auto raceId = npcData.GetProperty("raceId");
      raceId.GetType() == JsType::Number) {
    changeFormNpc->race = SaveFile_::ChangeFormNPC_::RaceChange();
    changeFormNpc->race->defaultRace =
      FormIdToRefId(static_cast<uint32_t>(static_cast<double>(raceId)));
    changeFormNpc->race->myRaceNow =
      FormIdToRefId(static_cast<uint32_t>(static_cast<double>(raceId)));
  }

  if (auto face = npcData.GetProperty("face");
      face.GetType() == JsType::Object) {
    changeFormNpc->face = SaveFile_::ChangeFormNPC_::Face();

    if (auto bodySkinColor = face.GetProperty("bodySkinColor");
        bodySkinColor.GetType() == JsType::Number) {
      changeFormNpc->face->bodySkinColor =
        RgbToAbgr(static_cast<int32_t>(bodySkinColor));
    }

    if (auto headPartIds = face.GetProperty("headPartIds");
        headPartIds.GetType() == JsType::Array) {
      int n = static_cast<int>(headPartIds.GetProperty("length"));

      for (int i = 0; i < n; ++i) {
        auto jHpId = headPartIds.GetProperty(i);
        auto hpId = static_cast<uint32_t>(static_cast<double>(jHpId));
        changeFormNpc->face->headParts.push_back(FormIdToRefId(hpId));
      }
    }

    if (auto presets = face.GetProperty("presets");
        presets.GetType() == JsType::Array) {
      int n = static_cast<int>(presets.GetProperty("length"));
      for (int i = 0; i < n; ++i) {
        auto jValue = presets.GetProperty(i);
        auto value = static_cast<uint32_t>(static_cast<double>(jValue));
        changeFormNpc->face->presets.push_back(value);
      }
    }

    if (auto headTextureSetId = face.GetProperty("headTextureSetId");
        headTextureSetId.GetType() == JsType::Number) {
      auto id = static_cast<uint32_t>(static_cast<double>(headTextureSetId));
      changeFormNpc->face->headTextureSet = FormIdToRefId(id);
    }
  }

  return changeFormNpc;
}

}

JsValue LoadGameApi::LoadGame(const JsFunctionArguments& args)
{
  std::array<float, 3> pos = JsExtractPoint(args[1]),
                       angle = JsExtractPoint(args[2]);
  uint32_t cellOrWorld = (uint32_t)(double)args[3];
  auto npcData = args[4];

  auto save = LoadGame::PrepareSaveFile();
  if (!save)
    throw NullPointerException("save");

  std::unique_ptr<SaveFile_::ChangeFormNPC_> changeFormNpc =
    CreateChangeFormNpc(save, npcData);

  LoadGame::Run(save, pos, angle, cellOrWorld, nullptr, nullptr,
                changeFormNpc.get());
  return JsValue::Undefined();
}
