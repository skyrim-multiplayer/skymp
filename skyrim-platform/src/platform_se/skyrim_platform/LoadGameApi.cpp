#include "LoadGameApi.h"
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
  std::string binType = formId >= 0x01000000 ? "10" : "01";
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
  if (npcData.GetType() != JsValue::Type::Object)
    return nullptr;

  std::unique_ptr<SaveFile_::ChangeFormNPC_> changeFormNpc;
  changeFormNpc.reset(new SaveFile_::ChangeFormNPC_);

  if (auto name = npcData.GetProperty("name");
      name.GetType() == JsValue::Type::String)
    changeFormNpc->playerName = static_cast<std::string>(name);

  if (auto raceId = npcData.GetProperty("raceId");
      raceId.GetType() == JsValue::Type::Number) {
    changeFormNpc->race = SaveFile_::ChangeFormNPC_::RaceChange();
    changeFormNpc->race->defaultRace =
      FormIdToRefId(static_cast<uint32_t>(static_cast<double>(raceId)));
    changeFormNpc->race->myRaceNow =
      FormIdToRefId(static_cast<uint32_t>(static_cast<double>(raceId)));
  }

  if (auto isFemale = npcData.GetProperty("isFemale");
      isFemale.GetType() == JsValue::Type::Boolean) {
    if (isFemale.ToString() == "true") {
      changeFormNpc->gender = isFemale ? 1 : 0;
    }
  }

  if (auto face = npcData.GetProperty("face");
      face.GetType() == JsValue::Type::Object) {
    changeFormNpc->face = SaveFile_::ChangeFormNPC_::Face();

    if (auto bodySkinColor = face.GetProperty("bodySkinColor");
        bodySkinColor.GetType() == JsValue::Type::Number) {
      changeFormNpc->face->bodySkinColor =
        RgbToAbgr(static_cast<int32_t>(bodySkinColor));
    }

    if (auto headPartIds = face.GetProperty("headPartIds");
        headPartIds.GetType() == JsValue::Type::Array) {
      int n = static_cast<int>(headPartIds.GetProperty("length"));

      for (int i = 0; i < n; ++i) {
        auto jHpId = headPartIds.GetProperty(i);
        auto hpId = static_cast<uint32_t>(static_cast<double>(jHpId));
        changeFormNpc->face->headParts.push_back(FormIdToRefId(hpId));
      }
    }

    if (auto presets = face.GetProperty("presets");
        presets.GetType() == JsValue::Type::Array) {
      int n = static_cast<int>(presets.GetProperty("length"));
      for (int i = 0; i < n; ++i) {
        auto jValue = presets.GetProperty(i);
        auto value = static_cast<uint32_t>(static_cast<double>(jValue));
        changeFormNpc->face->presets.push_back(value);
      }
    }

    if (auto headTextureSetId = face.GetProperty("headTextureSetId");
        headTextureSetId.GetType() == JsValue::Type::Number) {
      auto id = static_cast<uint32_t>(static_cast<double>(headTextureSetId));
      changeFormNpc->face->headTextureSet = FormIdToRefId(id);
    }
  }

  return changeFormNpc;
}

// ^^^ upper still needs to be ported to nodejs 

std::unique_ptr<LoadGame::Time> CreateTime(
  std::shared_ptr<SaveFile_::SaveFile>, Napi::Object time_)
{
  auto hours = NapiHelper::ExtractInt32(time_.Get("hours"), "time.hours");
  auto minutes = NapiHelper::ExtractInt32(time_.Get("minutes"), "time.minutes");
  auto seconds = NapiHelper::ExtractInt32(time_.Get("seconds"), "time.seconds");

  auto time = std::make_unique<LoadGame::Time>();
  time->Set(seconds, minutes, hours);
  return time;
}

std::unique_ptr<std::vector<std::string>> CreateLoadOrder(
  std::shared_ptr<SaveFile_::SaveFile>, Napi::Array loadOrder_)
{
  std::unique_ptr<std::vector<std::string>> loadOrder;
  loadOrder.reset(new std::vector<std::string>);
  int n = loadOrder_.Length();
  for (int i = 0; i < n; ++i) {
    auto jValue = loadOrder_.Get(i);
    auto value = NapiHelper::ExtractString(jValue, fmt::format("loadOrder[{}]", i));
    loadOrder->push_back(value);
  }
  return loadOrder;
}

}

Napi::Value LoadGameApi::LoadGame(const Napi::CallbackInfo& info)
{
  NiPoint3 niPos = NapiHelper::ExtractNiPoint3(info[0], "pos");
  NiPoint3 niAngle = NapiHelper::ExtractNiPoint3(info[1], "angle");
  std::array<float, 3> pos = {niPos[0], niPos[1], niPos[2]};
  std::array<float, 3> angle = {niAngle[0], niAngle[1], niAngle[2]};

  uint32_t cellOrWorld = NapiHelper::ExtractUInt32(info[2], "cellOrWorld");
  auto npcData = NapiHelper::ExtractObject(info[3], "npcData");
  auto loadOrder = NapiHelper::ExtractArray(info[4], "loadOrder");
  auto time = NapiHelper::ExtractObject(info[5], "time");

  constexpr auto kPathInAssetsMale = "assets/template.ess";

  const char* pathInAsset = kPathInAssetsMale;

  auto save = LoadGame::PrepareSaveFile(pathInAsset);
  if (!save) {
    throw NullPointerException("save");
  }

  std::unique_ptr<SaveFile_::ChangeFormNPC_> changeFormNpc =
    CreateChangeFormNpc(save, npcData);

  std::unique_ptr<std::vector<std::string>> saveLoadOrder =
    CreateLoadOrder(save, loadOrder);

  std::unique_ptr<LoadGame::Time> saveFileTime = CreateTime(save, time);

  const auto& _baseSavefile = save;
  const auto& _pos = pos;
  const auto& _angle = angle;
  const auto& _cellOrWorld = cellOrWorld;
  const auto& _time = saveFileTime.get();
  SaveFile_::Weather* _weather = nullptr;
  SaveFile_::ChangeFormNPC_* _changeFormNPC = changeFormNpc.get();
  std::vector<std::string>* _loadOrder = saveLoadOrder.get();
  LoadGame::Run(_baseSavefile, _pos, _angle, _cellOrWorld, _time, _weather,
                _changeFormNPC, _loadOrder);

  return info.Env().Undefined();
}
