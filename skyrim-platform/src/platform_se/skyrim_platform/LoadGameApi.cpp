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
  std::shared_ptr<SaveFile_::SaveFile> save, Napi::Object npcData)
{
  auto changeFormNpc = std::make_unique<SaveFile_::ChangeFormNPC_>();

  if (auto name = npcData.Get("name"); !name.IsUndefined() && !name.IsNull()) {
    changeFormNpc->playerName = NapiHelper::ExtractString(name, "npcData.name");
  }

  if (auto raceId = npcData.Get("raceId"); !raceId.IsUndefined() && !raceId.IsNull()) {
    auto raceIdExtracted = NapiHelper::ExtractUInt32(raceId, "npcData.raceId");
    changeFormNpc->race = SaveFile_::ChangeFormNPC_::RaceChange();
    changeFormNpc->race->defaultRace = FormIdToRefId(raceIdExtracted);
    changeFormNpc->race->myRaceNow = FormIdToRefId(raceIdExtracted);
  }

  // TODO: why mismatch with skyrimPlatform.ts: instead of 'npcData' this is in 'npcData.face'???
  if (auto isFemale = npcData.Get("isFemale"); !isFemale.IsUndefined() && !isFemale.IsNull()) {
    // TODO: this is a hotfix, fix properly.
    // To test ensure players after relog preserve their gender anims
    if (static_cast<std::string>(isFemale.ToString()) == "true") {
      changeFormNpc->gender = isFemale ? 1 : 0;
    }
  }

  if (auto face = npcData.Get("face"); !face.IsUndefined() && !face.IsNull()) {
    changeFormNpc->face = SaveFile_::ChangeFormNPC_::Face();

    auto faceExtracted = NapiHelper::ExtractObject(face, "npcData.face");

    if (auto bodySkinColor = faceExtracted.Get("bodySkinColor");
        !bodySkinColor.IsUndefined() && !bodySkinColor.IsNull()) {
      auto bodySkinColorExtracted = NapiHelper::ExtractInt32(bodySkinColor, "npcData.bodySkinColor");
      changeFormNpc->face->bodySkinColor = RgbToAbgr(bodySkinColorExtracted);
    }

    if (auto headPartIds = faceExtracted.Get("headPartIds");
        !headPartIds.IsUndefined() && !headPartIds.IsNull()) {
      auto headPartIdsExtracted = NapiHelper::ExtractArray(headPartIds, "npcData.headPartIds");
      int n = headPartIdsExtracted.Length();

      for (int i = 0; i < n; ++i) {
        auto jHpId = headPartIdsExtracted.Get(i);
        std::string comment = fmt::format("npcData.headPartIds[{}]", i);
        auto hpId = NapiHelper::ExtractUInt32(jHpId, comment.data());
        changeFormNpc->face->headParts.push_back(FormIdToRefId(hpId));
      }
    }

    if (auto presets = faceExtracted.Get("presets");
        !presets.IsUndefined() && !presets.IsNull()) {
      auto presetsExtracted = NapiHelper::ExtractArray(presets, "npcData.presets");
      int n = presetsExtracted.Length();
      for (int i = 0; i < n; ++i) {
        auto jValue = presetsExtracted.Get(i);
        std::string comment = fmt::format("npcData.presets[{}]", i);
        auto value = NapiHelper::ExtractUInt32(jValue, comment.data());
        changeFormNpc->face->presets.push_back(value);
      }
    }

    if (auto headTextureSetId = faceExtracted.Get("headTextureSetId"); !headTextureSetId.IsUndefined() && !headTextureSetId.IsNull()) {
      auto id = NapiHelper::ExtractUInt32(headTextureSetId, "npcData.headTextureSetId");
      changeFormNpc->face->headTextureSet = FormIdToRefId(id);
    }
  }

  return changeFormNpc;
}

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
    std::string comment = fmt::format("loadOrder[{}]", i);
    auto value = NapiHelper::ExtractString(jValue, comment.data());
    loadOrder->push_back(value);
  }
  return loadOrder;
}

}

Napi::Value LoadGameApi::LoadGame(const Napi::CallbackInfo& info)
{
  NiPoint3 niPos = NapiHelper::ExtractNiPoint3(info[0], "pos");
  NiPoint3 niAngle = NapiHelper::ExtractNiPoint3(info[1], "angle");
  std::array<float, 3> pos = { niPos[0], niPos[1], niPos[2] };
  std::array<float, 3> angle = { niAngle[0], niAngle[1], niAngle[2] };

  uint32_t cellOrWorld = NapiHelper::ExtractUInt32(info[2], "cellOrWorld");

  constexpr auto kPathInAssetsMale = "assets/template.ess";

  const char* pathInAsset = kPathInAssetsMale;

  auto save = LoadGame::PrepareSaveFile(pathInAsset);
  if (!save) {
    throw NullPointerException("save");
  }

  std::unique_ptr<SaveFile_::ChangeFormNPC_> changeFormNpc =
    (info[3].IsUndefined() || info[3].IsNull()) ? nullptr : CreateChangeFormNpc(save, NapiHelper::ExtractObject(info[3], "npcData"));

  std::unique_ptr<std::vector<std::string>> saveLoadOrder =
    (info[4].IsUndefined() || info[4].IsNull()) ? nullptr : CreateLoadOrder(save, NapiHelper::ExtractArray(info[4], "loadOrder"));

  std::unique_ptr<LoadGame::Time> saveFileTime = 
    (info[5].IsUndefined() || info[5].IsNull()) ? nullptr : CreateTime(save, NapiHelper::ExtractObject(info[5], "time"));

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
