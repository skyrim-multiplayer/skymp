#include "ZipDatabase.h"

#include "libzippp/libzippp.h"
#include <FileUtils.h>
#include <MappedBuffer.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>

struct ZipDatabase::Impl
{
  Impl(std::string filePath_, std::shared_ptr<spdlog::logger> logger_)
    : filePath(std::move(filePath_))
    , logger(std::move(logger_))
  {
  }

  std::string filePath;
  std::shared_ptr<spdlog::logger> logger;
};

ZipDatabase::ZipDatabase(std::string filePath_,
                         std::shared_ptr<spdlog::logger> logger_)
  : pImpl(std::make_shared<Impl>(std::move(filePath_), std::move(logger_)))
{
}

ZipDatabase::~ZipDatabase() = default;

std::vector<std::optional<MpChangeForm>>&& ZipDatabase::UpsertImpl(
  std::vector<std::optional<MpChangeForm>>&& changeForms,
  size_t& outNumUpserted)
{
  try {
    auto filePathAbsolute =
      std::filesystem::absolute(pImpl->filePath).string();

    libzippp::ZipArchive archive(filePathAbsolute.data());

    archive.open(libzippp::ZipArchive::Write);

    std::vector<std::unique_ptr<std::string>> buffers;

    for (auto& changeForm : changeForms) {
      if (changeForm == std::nullopt) {
        continue;
      }

      // Data to be added or updated
      std::string data = MpChangeForm::ToJson(*changeForm).dump(2);
      std::string fileName = changeForm->formDesc.ToString('_') + ".json";
      std::string filePath = fileName;

      buffers.push_back(std::make_unique<std::string>(data));

      // Add new file or replace existing one
      archive.addData(filePath, buffers.back()->data(),
                      buffers.back()->size());
    }

    archive.close();

    outNumUpserted = changeForms.size();

    return std::move(changeForms);
  } catch (std::exception& e) {
    throw Viet::UpsertFailedException(std::move(changeForms), e.what());
  }
}

void ZipDatabase::Iterate(const IterateCallback& iterateCallback,
                          std::optional<std::vector<FormDesc>> filter)
{
  auto filePathAbsolute = std::filesystem::absolute(pImpl->filePath).string();

  libzippp::ZipArchive archive(filePathAbsolute.data());

  archive.open(libzippp::ZipArchive::ReadOnly);

  std::vector<libzippp::ZipEntry> entries = archive.getEntries();
  std::vector<libzippp::ZipEntry>::iterator it;

  simdjson::dom::parser p;

  std::optional<std::unordered_set<std::string>> filterSet;
  if (filter) {
    std::unordered_set<std::string>& value = filterSet.emplace();
    for (const auto& desc : *filter) {
      value.insert(desc.ToString());
    }
  }

  for (it = entries.begin(); it != entries.end(); ++it) {
    libzippp::ZipEntry entry = *it;
    std::string name = entry.getName();
    int size = entry.getSize();

    std::string textData = entry.readAsText();

    try {
      auto result = p.parse(textData).value();
      auto changeForm = MpChangeForm::JsonToChangeForm(result);

      if (filterSet) {
        if (filterSet->find(changeForm.formDesc.ToString()) ==
            filterSet->end()) {
          continue;
        }
      }

      iterateCallback(changeForm);
    } catch (std::exception& e) {
      pImpl->logger->error("Parsing or loading of {} failed with {}", name,
                           e.what());
    }
  }
}
