#include "FileDatabase.h"
#include <filesystem>
#include <fstream>
#include <unordered_set>

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

std::vector<std::optional<MpChangeForm>>&& FileDatabase::UpsertImpl(
  std::vector<std::optional<MpChangeForm>>&& changeForms,
  size_t& outNumUpserted)
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

      bool wasOpen = f.is_open();

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

      if (!wasOpen) {
        throw std::runtime_error(
          fmt::format("Unable to open file {}", filePath.string()));
      } else if (!f) {
        throw std::runtime_error(fmt::format(
          "Unknown error while writing file {}", filePath.string()));
      } else {
        ++nUpserted;
      }
    }

    outNumUpserted = nUpserted;
    return std::move(changeForms);
  } catch (std::exception& e) {
    throw Viet::UpsertFailedException(std::move(changeForms), e.what());
  }
}

void FileDatabase::Iterate(const IterateCallback& iterateCallback,
                           std::optional<std::vector<FormDesc>> filter)
{
  auto p = pImpl->changeFormsDirectory;

  simdjson::dom::parser parser;

  if (!std::filesystem::exists(p)) {
    return;
  }

  std::optional<std::unordered_set<std::string>> filterSet;
  if (filter) {
    std::unordered_set<std::string>& value = filterSet.emplace();
    for (const auto& desc : *filter) {
      value.insert(desc.ToString());
    }
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
      auto changeForm = MpChangeForm::JsonToChangeForm(result);

      if (filterSet) {
        if (filterSet->find(changeForm.formDesc.ToString()) ==
            filterSet->end()) {
          continue;
        }
      }

      iterateCallback(changeForm);
    } catch (std::exception& e) {
      pImpl->logger->error("Parsing of {} failed with {}",
                           entry.path().string(), e.what());
    }
  }
}
