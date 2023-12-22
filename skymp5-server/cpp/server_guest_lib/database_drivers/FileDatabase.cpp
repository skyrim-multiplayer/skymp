#include "FileDatabase.h"
#include <filesystem>
#include <fstream>

struct FileDatabase::Impl
{
  const std::filesystem::path changeFormsDirectory;
  const std::shared_ptr<spdlog::logger> logger;
};

FileDatabase::FileDatabase(std::string directory_,
                           std::shared_ptr<spdlog::logger> logger_)
{
  std::filesystem::path p = directory_;
  p /= "changeForms";

  pImpl.reset(new Impl{ p, logger_ });
  std::filesystem::create_directories(p);
}

size_t FileDatabase::Upsert(const std::vector<MpChangeForm>& changeForms)
{
  std::filesystem::path p = pImpl->changeFormsDirectory;
  size_t nUpserted = 0;

  for (auto& changeForm : changeForms) {
    std::string fileName = changeForm.formDesc.ToString('_') + ".json";
    auto tempFilePath = p / (fileName + ".tmp"), filePath = p / fileName;

    std::ofstream f(tempFilePath);
    if (f) {
      f << MpChangeForm::ToJson(changeForm).dump(2);
    }

    if (!f.fail()) {
      f.close(); // otherwise rename fails on Windows
      std::filesystem::rename(tempFilePath, filePath);
    }

    if (!f.is_open()) {
      pImpl->logger->error("Unable to open file {}", filePath.string());
    } else if (!f) {
      pImpl->logger->error("Unknown error while writing file {}",
                           filePath.string());
    } else {
      ++nUpserted;
    }
  }

  return nUpserted;
}

void FileDatabase::Iterate(const IterateCallback& iterateCallback)
{
  auto p = pImpl->changeFormsDirectory;

  simdjson::dom::parser parser;

  if (!std::filesystem::exists(p)) {
    return;
  }

  for (auto& entry : std::filesystem::directory_iterator(p)) {
    try {
      std::ifstream t(entry.path());
      std::string jsonDump((std::istreambuf_iterator<char>(t)),
                           std::istreambuf_iterator<char>());

      auto result = parser.parse(jsonDump).value();
      iterateCallback(MpChangeForm::JsonToChangeForm(result));
    } catch (std::exception& e) {
      pImpl->logger->error("Parsing of {} failed with {}",
                           entry.path().string(), e.what());
    }
  }
}
