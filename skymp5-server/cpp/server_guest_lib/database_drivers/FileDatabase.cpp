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

size_t FileDatabase::Upsert(
  std::vector<std::optional<MpChangeForm>>&& changeForms)
{
  try {
    std::filesystem::path p = pImpl->changeFormsDirectory;
    size_t nUpserted = 0;

    for (auto& changeForm : changeForms) {
      if (changeForm == std::nullopt) {
        continue;
      }

      std::string fileName = changeForm->formDesc.ToString('_') + ".json";
      auto tempFilePath = p / (fileName + ".tmp"), filePath = p / fileName;

      std::ofstream f(tempFilePath);
      if (f) {
        f << MpChangeForm::ToJson(*changeForm).dump(2);
      }

      if (!f.fail()) {
        f.close();

        std::error_code errorCode;
        std::filesystem::rename(tempFilePath, filePath, errorCode);
        if (errorCode) {
          throw std::runtime_error(
            fmt::format("Unable to rename {} to {}: {}", tempFilePath.string(),
                        filePath.string(), errorCode.message()));
        }
      }

      if (!f.is_open()) {
        throw std::runtime_error(
          fmt::format("Unable to open file {}", filePath.string()));
      } else if (!f) {
        throw std::runtime_error(fmt::format(
          "Unknown error while writing file {}", filePath.string()));
      } else {
        ++nUpserted;
      }
    }

    return nUpserted;
  } catch (std::exception& e) {
    throw UpsertFailedException(std::move(changeForms), e.what());
  }
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
      if (entry.path().extension() != ".json") {
        continue;
      }

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
