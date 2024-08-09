#include "ZipDatabase.h"

#include "FileUtils.h"
#include "MappedBuffer.h"
#include "libzippp/libzippp.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <zlib.h>

namespace {
inline uint32_t ZlibGetCRC32Checksum(const void* readBuffer, z_size_t length)
{
  uLong hash = crc32_z(0L, Z_NULL, 0);
  hash = crc32_z(hash, static_cast<const Bytef*>(readBuffer), length);
  return static_cast<uint32_t>(hash);
}
}

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

size_t ZipDatabase::Upsert(
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

  return changeForms.size();
}

void ZipDatabase::Iterate(const IterateCallback& iterateCallback)
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
