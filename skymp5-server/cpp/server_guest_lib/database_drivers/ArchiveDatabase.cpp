#include "ArchiveDatabase.h"

#include "libzippp/libzippp.h"

#include <filesystem>

struct ArchiveDatabase::Impl
{
  Impl(std::string filePath_, std::shared_ptr<spdlog::logger> logger_)
    : filePath(std::move(filePath_))
    , logger(std::move(logger_))
  {
  }

  std::string filePath;
  std::shared_ptr<spdlog::logger> logger;
};

ArchiveDatabase::ArchiveDatabase(std::string filePath_,
                                 std::shared_ptr<spdlog::logger> logger_)
  : pImpl(std::make_shared<Impl>(std::move(filePath_), std::move(logger_)))
{
}

ArchiveDatabase::~ArchiveDatabase() = default;

UpsertResult ArchiveDatabase::Upsert(
  std::vector<std::optional<MpChangeForm>>&& changeForms)
{
  auto filePathAbsolute = std::filesystem::absolute(pImpl->filePath).string();

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
    archive.addData(filePath, buffers.back()->data(), buffers.back()->size());
  }

  archive.close();

  return { changeForms.size(), std::nullopt };
}

void ArchiveDatabase::Iterate(const IterateCallback& iterateCallback)
{
  auto filePathAbsolute = std::filesystem::absolute(pImpl->filePath).string();

  libzippp::ZipArchive archive(filePathAbsolute.data());

  archive.open(libzippp::ZipArchive::ReadOnly);

  std::vector<libzippp::ZipEntry> entries = archive.getEntries();
  std::vector<libzippp::ZipEntry>::iterator it;

  simdjson::dom::parser p;

  for (it = entries.begin(); it != entries.end(); ++it) {
    libzippp::ZipEntry entry = *it;
    std::string name = entry.getName();
    int size = entry.getSize();

    std::string textData = entry.readAsText();

    try {
      auto result = p.parse(textData).value();
      iterateCallback(MpChangeForm::JsonToChangeForm(result));
    } catch (std::exception& e) {
      pImpl->logger->error("Parsing or loading of {} failed with {}", name,
                           e.what());
    }
  }
}
