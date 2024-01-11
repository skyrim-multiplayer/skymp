#pragma once

namespace SaveFile_ {
struct PlayerLocation;
struct RefID;
struct SaveFile;
struct ChangeForm;
struct ChangeFormNPC_;
struct Weather;
struct GlobalVariables;
}

class LoadGame
{
public:
  class Time
  {
  public:
    void Set(uint8_t inS, uint8_t inM, uint8_t inH)
    {
      seconds = inS;
      minutes = inM;
      hours = inH;
      hasData = true;
    }

    bool IsSet(void) { return hasData; }

    uint8_t GetSeconds(void) { return seconds; }
    uint8_t GetMinutes(void) { return minutes; }
    uint8_t GetHours(void) { return hours; }

  private:
    uint8_t seconds = 0, minutes = 0, hours = 0;
    bool hasData = false;
  };

  static std::shared_ptr<SaveFile_::SaveFile> PrepareSaveFile();

  static void Run(std::shared_ptr<SaveFile_::SaveFile> baseSavefile,
                  const std::array<float, 3>& pos,
                  const std::array<float, 3>& angle, uint32_t cellOrWorld,
                  Time* time = nullptr, SaveFile_::Weather* _weather = nullptr,
                  SaveFile_::ChangeFormNPC_* changeFormNPC = nullptr,
                  std::vector<std::string>* loadOrder = nullptr);

  static std::wstring GetPathToMyDocuments();

private:
  static std::wstring StringToWstring(const std::string& s);

  static std::string GenerateGuid();

  static std::filesystem::path GetSaveFullPath(const std::string& name);

  static SaveFile_::PlayerLocation* FindSectionWithPlayerLocation(
    std::shared_ptr<SaveFile_::SaveFile> save);

  static SaveFile_::PlayerLocation CreatePlayerLocation(
    const std::array<float, 3>& pos, const SaveFile_::RefID& world);

  static std::vector<uint8_t> Decompress(
    const SaveFile_::ChangeForm& changeForm);

  static void EditChangeForm(std::vector<uint8_t>& data,
                             const std::array<float, 3>& pos,
                             const std::array<float, 3>& angle,
                             const SaveFile_::RefID& world);

  static std::vector<uint8_t> Compress(
    const std::vector<uint8_t>& uncompressed);

  static void WriteChangeForm(std::shared_ptr<SaveFile_::SaveFile> save,
                              SaveFile_::ChangeForm& changeForm,
                              const std::vector<uint8_t>& compressed,
                              size_t uncompressedSize);

  static void ModifyEssStructure(std::shared_ptr<SaveFile_::SaveFile> save,
                                 std::array<float, 3> pos,
                                 std::array<float, 3> angle,
                                 uint32_t cellOrWorld);

  static void ModifyPluginInfo(std::shared_ptr<SaveFile_::SaveFile>& save);

  static void ModifySaveTime(std::shared_ptr<SaveFile_::SaveFile>& save,
                             Time* time);

  static void ModifySaveWeather(std::shared_ptr<SaveFile_::SaveFile>& save,
                                SaveFile_::Weather* _weather);

  static void ModifyPlayerFormNPC(std::shared_ptr<SaveFile_::SaveFile> save,
                                  SaveFile_::ChangeFormNPC_* changeFormNPC);

  static void ModifyLoadOrder(std::shared_ptr<SaveFile_::SaveFile> save,
                              std::vector<std::string>* loadOrder);

  static void FillChangeForm(
    std::shared_ptr<SaveFile_::SaveFile> save, SaveFile_::ChangeForm* form,
    std::pair<uint32_t, std::vector<uint8_t>>& newValues);
};
