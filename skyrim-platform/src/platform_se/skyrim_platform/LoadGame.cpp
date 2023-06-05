#pragma comment(lib, "shell32.lib")
#include "LoadGame.h"
#include "NullPointerException.h"
#include "PapyrusTESModPlatform.h"
#include "savefile/SFChangeFormNPC.h"
#include "savefile/SFReader.h"
#include "savefile/SFSeekerOfDifferences.h"
#include "savefile/SFWriter.h"

namespace fs = std::filesystem;

CMRC_DECLARE(skyrim_plugin_resources);

constexpr auto g_saveFilePrefix = "TESMODPLATFORM-";

class LoadGameEventSink : public RE::BSTEventSink<RE::TESLoadGameEvent>
{
public:
  LoadGameEventSink()
  {
    auto holder = RE::ScriptEventSourceHolder::GetSingleton();
    if (!holder) {
      throw NullPointerException("holder");
    }

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESLoadGameEvent>*>(this));
  }

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESLoadGameEvent* event,
    RE::BSTEventSource<RE::TESLoadGameEvent>* eventSource) override
  {
    std::thread([] {
      // A way to wait 5 seconds game time
      for (int i = 0; i < 50; ++i) {
        auto n = TESModPlatform::GetNumPapyrusUpdates();
        while (n == TESModPlatform::GetNumPapyrusUpdates()) {
          Sleep(20);
        }
        Sleep(80);
      }

      // Allow MoveRefrToPosition again
      TESModPlatform::BlockMoveRefrToPosition(false);

      // Removes our temporary save files
      try {
        std::filesystem::path path = LoadGame::GetPathToMyDocuments() +
          L"\\My Games\\Skyrim Special Edition\\Saves\\";

        for (auto& file : std::filesystem::directory_iterator(path)) {
          if (file.path().filename().generic_string().find(g_saveFilePrefix) !=
              std::string::npos)
            try {
              std::filesystem::remove(file);
            } catch (...) {
              // I have no idea how to handle these exceptions properly
            }
        }
      } catch (...) {
      }
    }).detach();
    return RE::BSEventNotifyControl::kContinue;
  }
};

extern bool g_allowHideMainMenu;

std::shared_ptr<SaveFile_::SaveFile> LoadGame::PrepareSaveFile()
{
  cmrc::file file;
  try {
    file = cmrc::skyrim_plugin_resources::get_filesystem().open(
      "assets/template.ess");
  } catch (std::exception& e) {
    auto dir =
      cmrc::skyrim_plugin_resources::get_filesystem().iterate_directory("");
    std::stringstream ss;
    ss << e.what() << std::endl << std::endl;
    ss << "Root directory contents is: " << std::endl;
    for (auto entry : dir) {
      ss << entry.filename() << std::endl;
    }
    throw std::runtime_error(ss.str());
  }
  return SaveFile_::Reader((uint8_t*)(file.begin()), file.size())
    .GetStructure();
}

void LoadGame::Run(std::shared_ptr<SaveFile_::SaveFile> save,
                   const std::array<float, 3>& pos,
                   const std::array<float, 3>& angle, uint32_t cellOrWorld,
                   Time* time, SaveFile_::Weather* _weather,
                   SaveFile_::ChangeFormNPC_* changeFormNPC)
{
  if (!save) {
    throw std::runtime_error("Bad SaveFile");
  }

  ModifySaveTime(save, time);
  ModifySaveWeather(save, _weather);
  ModifyPluginInfo(save);
  ModifyPlayerFormNPC(save, changeFormNPC);
  ModifyEssStructure(save, pos, angle, cellOrWorld);

  auto name = g_saveFilePrefix + GenerateGuid();
  if (!SaveFile_::Writer(save).CreateSaveFile(GetSaveFullPath(name))) {
    throw std::runtime_error("CreateSaveFile failed");
  }

  TESModPlatform::BlockMoveRefrToPosition(true);
  static LoadGameEventSink g_sink;

  if (auto saveLoadManager = RE::BGSSaveLoadManager::GetSingleton()) {
    return saveLoadManager->Load(name.data());
  } else {
    throw NullPointerException("saveLoadManager");
  }
}

fs::path LoadGame::GetSaveFullPath(const std::string& name)
{
  return GetPathToMyDocuments() +
    L"\\My Games\\Skyrim Special Edition\\Saves\\" + StringToWstring(name) +
    L".ess";
}

std::wstring LoadGame::GetPathToMyDocuments()
{
  PWSTR ppszPath;
  HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &ppszPath);
  std::wstring myPath;
  if (SUCCEEDED(hr)) {
    myPath = ppszPath;
  }
  CoTaskMemFree(ppszPath);
  return myPath;
}

void LoadGame::ModifyPluginInfo(std::shared_ptr<SaveFile_::SaveFile>& save)
{
  std::vector<std::string> newPlugins;
  auto dataHandler = RE::TESDataHandler::GetSingleton();

  if (!dataHandler) {
    throw NullPointerException("dataHandler");
  }

  for (auto it = dataHandler->files.begin(); it != dataHandler->files.end();
       ++it)
    newPlugins.push_back(std::string((*it)->fileName));

  save->OverwritePluginInfo(newPlugins);
}

void LoadGame::ModifySaveTime(std::shared_ptr<SaveFile_::SaveFile>& save,
                              LoadGame::Time* time)
{
  if (!time) {
    return;
  }

  if (!time->IsSet()) {
    throw std::runtime_error("Time data is not filled");
  }

  SaveFile_::RefID gameHourID = 0x38;

  auto index = save->FindIndexInFormIdArray(0x38);
  if (index >= 0) {
    gameHourID = SaveFile_::RefID(static_cast<uint32_t>(index));
  }

  auto var = save->GetGlobalvariableByRefID(gameHourID);
  if (!var) {
    throw std::runtime_error("Global Varible not found");
  }

  var->value =
    time->GetHours() + time->GetMinutes() / 60.0 + time->GetSeconds() / 3600.0;
}

void LoadGame::ModifySaveWeather(std::shared_ptr<SaveFile_::SaveFile>& save,
                                 SaveFile_::Weather* _weather)
{
  if (!_weather) {
    return;
  }

  SaveFile_::GlobalData& gData =
    save->globalDataTable1[SaveFile_::SaveFile::WEATHER_INDEX];

  if (gData.type != SaveFile_::SaveFile::WEATHER_INDEX) {
    throw std::runtime_error("Wrong weather index");
  }

  SaveFile_::Weather* weather =
    reinterpret_cast<SaveFile_::Weather*>(gData.data.get());

  if (!weather) {
    throw NullPointerException("weather");
  }

  weather->climate = _weather->climate;
  weather->weather = _weather->weather;
  weather->regnWeather = _weather->regnWeather;
  weather->weatherPct = _weather->weatherPct;
}

void LoadGame::ModifyPlayerFormNPC(std::shared_ptr<SaveFile_::SaveFile> save,
                                   SaveFile_::ChangeFormNPC_* changeFormNPC)
{
  using namespace SaveFile_;
  if (!changeFormNPC) {
    return;
  }

  auto form = save->GetChangeFormByRefID(RefID(RefID::PlayerBase),
                                         uint8_t(ChangeForm::Type::NPC));

  if (form) {
    auto newForm = changeFormNPC->ToBinary();
    FillChangeForm(save, form, newForm);
  }
}

void LoadGame::FillChangeForm(
  std::shared_ptr<SaveFile_::SaveFile> save, SaveFile_::ChangeForm* form,
  std::pair<uint32_t, std::vector<uint8_t>>& newValues)
{

  save->fileLocationTable.formIDArrayCountOffset -= form->length1;
  save->fileLocationTable.formIDArrayCountOffset += newValues.second.size();

  save->fileLocationTable.unknownTable3Offset -= form->length1;
  save->fileLocationTable.unknownTable3Offset += newValues.second.size();

  save->fileLocationTable.globalDataTable3Offset -= form->length1;
  save->fileLocationTable.globalDataTable3Offset += newValues.second.size();

  form->length2 = 0;
  form->length1 = newValues.second.size();
  form->data = newValues.second;
  form->changeFlags = newValues.first;
}

void LoadGame::ModifyEssStructure(std::shared_ptr<SaveFile_::SaveFile> save,
                                  std::array<float, 3> pos,
                                  std::array<float, 3> angle,
                                  uint32_t cellOrWorld)
{
  auto playerLoc = FindSectionWithPlayerLocation(save);
  if (!playerLoc) {
    throw std::runtime_error("Couldn't find PlayerLocation in the save file");
  }

  auto worldRefId = SaveFile_::RefID::CreateRefId(*save, cellOrWorld);
  *playerLoc = CreatePlayerLocation(pos, worldRefId);

  auto player = std::find_if(
    save->changeForms.begin(), save->changeForms.end(),
    [](auto& changeForm) { return changeForm.formID.IsPlayerID(); });
  if (player == save->changeForms.end()) {
    throw std::runtime_error("Unable to find Player's change form");
  }
  bool isCompressed = player->length2 > 0;
  if (!isCompressed) {
    throw std::runtime_error("Player's ChangeForm must be compressed");
  }

  auto uncompressed = Decompress(*player);
  EditChangeForm(uncompressed, pos, angle, worldRefId);
  auto compressed = Compress(uncompressed);
  WriteChangeForm(save, *player, compressed, uncompressed.size());
}

SaveFile_::PlayerLocation* LoadGame::FindSectionWithPlayerLocation(
  std::shared_ptr<SaveFile_::SaveFile> save)
{
  auto& c = save->globalDataTable1;
  auto it = std::find_if(
    c.begin(), c.end(), [](const SaveFile_::GlobalData& globalData) {
      return globalData.type == SaveFile_::PlayerLocation::GlobalDataType;
    });
  if (it == c.end()) {
    return nullptr;
  }
  return static_cast<SaveFile_::PlayerLocation*>(it->data.get());
}

SaveFile_::PlayerLocation LoadGame::CreatePlayerLocation(
  const std::array<float, 3>& pos, const SaveFile_::RefID& world)
{
  SaveFile_::PlayerLocation r;
  r.nextObjectId = 4278195454;
  r.worldspace1 = world;
  r.coorX = (int)pos[0] / 4096;
  r.coorY = (int)pos[1] / 4096;
  r.worldspace2 = world;
  r.posX = pos[0];
  r.posY = pos[1];
  r.posZ = pos[2];
  r.unknown = 0;
  return r;
}

std::vector<uint8_t> LoadGame::Decompress(
  const SaveFile_::ChangeForm& changeForm)
{
  const size_t lengthCompressed = changeForm.length1;
  const size_t lengthUncompressed = changeForm.length2;
  auto uncompressed = (std::vector<uint8_t>(lengthUncompressed));
  const auto compressed = changeForm.data.data();
  SaveFile_::SeekerOfDifferences::ZlibDecompress(
    compressed, lengthCompressed, uncompressed.data(), lengthUncompressed);
  return uncompressed;
}

void LoadGame::EditChangeForm(std::vector<uint8_t>& data,
                              const std::array<float, 3>& pos,
                              const std::array<float, 3>& angle,
                              const SaveFile_::RefID& world)
{
  auto d = data.data();
  *reinterpret_cast<SaveFile_::RefID*>(d + 0) = world;

  auto& changeFormPos = *reinterpret_cast<std::array<float, 3>*>(d + 3);
  changeFormPos = pos;

  float* changeFormAngle = reinterpret_cast<float*>(d + 15);
  for (int i = 0; i < 3; ++i) {
    changeFormAngle[i] = angle[i] / 180.f * acos(-1);
  }
}

std::vector<uint8_t> LoadGame::Compress(
  const std::vector<uint8_t>& uncompressed)
{
  size_t newCompressedSizeMax = uncompressed.size();
  std::vector<uint8_t> newCompressed(newCompressedSizeMax, 0);
  const auto newCompressedSize = SaveFile_::SeekerOfDifferences::ZlibCompress(
    uncompressed.data(), uncompressed.size(), newCompressed.data(),
    newCompressedSizeMax);
  newCompressed.resize(newCompressedSize);
  return newCompressed;
}

void LoadGame::WriteChangeForm(std::shared_ptr<SaveFile_::SaveFile> save,
                               SaveFile_::ChangeForm& changeForm,
                               const std::vector<uint8_t>& compressed,
                               size_t uncompressedSize)
{
  auto previousSize = changeForm.length1;

  changeForm.length1 = compressed.size();
  changeForm.length2 = uncompressedSize;
  changeForm.data.resize(changeForm.length1);
  std::copy(compressed.begin(), compressed.end(), changeForm.data.begin());

  // fix offsets
  const auto diff = (int64_t)previousSize - (int64_t)compressed.size();
  save->fileLocationTable.formIDArrayCountOffset -= diff;
  save->fileLocationTable.unknownTable3Offset -= diff;
  save->fileLocationTable.globalDataTable3Offset -= diff;
}

std::wstring LoadGame::StringToWstring(std::string s)
{
  std::wstring ws(s.size(), L' ');
  auto n = std::mbstowcs(&ws[0], s.c_str(), s.size());
  ws.resize(n);
  return ws;
}

std::string LoadGame::GenerateGuid()
{
  GUID guid;
  if (CoCreateGuid(&guid) != S_OK) {
    throw std::runtime_error("CoCreateGuid failed");
  }

  char name[MAX_PATH] = { 0 };
  sprintf_s(
    name,
    "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
    guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
    guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6],
    guid.Data4[7]);
  return name;
}
