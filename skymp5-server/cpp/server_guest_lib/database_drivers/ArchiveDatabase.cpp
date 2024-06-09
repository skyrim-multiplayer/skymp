#include "ArchiveDatabase.h"

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

constexpr auto kDbHashFileName = "_dbhash";
constexpr auto kChecksumExtension = ".checksum";
constexpr auto kCrc32Key = "crc32_checksum";

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

    if (name == kDbHashFileName) {
      continue;
    }

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

void ArchiveDatabase::SetDbHash(const std::string& data)
{
  auto filePathAbsolute = std::filesystem::absolute(pImpl->filePath).string();

  libzippp::ZipArchive archive(filePathAbsolute.data());

  archive.open(libzippp::ZipArchive::Write);

  // Add new file or replace existing one
  archive.addData(kDbHashFileName, data.data(), data.size());

  // needs to be done before buffer invalidation
  archive.close();
}

std::optional<std::string> ArchiveDatabase::GetDbHash() const
{
  auto filePathAbsolute = std::filesystem::absolute(pImpl->filePath).string();

  libzippp::ZipArchive archive(filePathAbsolute.data());

  archive.open(libzippp::ZipArchive::ReadOnly);

  std::vector<libzippp::ZipEntry> entries = archive.getEntries();
  std::vector<libzippp::ZipEntry>::iterator it;

  for (it = entries.begin(); it != entries.end(); ++it) {
    libzippp::ZipEntry entry = *it;
    std::string name = entry.getName();

    if (name == kDbHashFileName) {
      return entry.readAsText();
    }
  }

  return std::nullopt;
}

uint32_t ArchiveDatabase::GetArchiveChecksum() const
{
  if (std::filesystem::exists(pImpl->filePath)) {
    Viet::MappedBuffer mappedBuffer(pImpl->filePath);
    return ZlibGetCRC32Checksum(mappedBuffer.GetData(),
                                mappedBuffer.GetLength());
  }
  return 0;
}

std::optional<uint32_t> ArchiveDatabase::ReadArchiveChecksumExpected() const
{
  auto checksumFilePath = pImpl->filePath + kChecksumExtension;
  std::string fileContents;

  try {
    fileContents = Viet::ReadFileIntoString(checksumFilePath);
  } catch (std::exception& e) {
    pImpl->logger->info("Unable to read archive checksum: {}", e.what());
    return std::nullopt;
  }

  nlohmann::json document;

  try {
    document = nlohmann::json::parse(fileContents);
  } catch (nlohmann::json::parse_error& e) {
    pImpl->logger->warn("Failed to parse archive checksum: {}", e.what());
    return std::nullopt;
  }

  uint32_t checksum = 0;

  try {
    checksum = document[kCrc32Key].get<uint32_t>();
  } catch (nlohmann::json::exception& e) {
    pImpl->logger->warn("Failed to read archive checksum: {}", e.what());
    return std::nullopt;
  }

  return checksum;
}

void ArchiveDatabase::WriteArchiveChecksumExpected(uint32_t checksum)
{
  nlohmann::json document{ { kCrc32Key, checksum } };

  auto filePathAbsolute =
    std::filesystem::absolute(pImpl->filePath + kChecksumExtension).string();

  std::fstream file(filePathAbsolute, std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    pImpl->logger->error("Failed to open file {}", filePathAbsolute);
    return;
  }

  file << document.dump(2);
}

void ArchiveDatabase::Unlink()
{
  auto filePathAbsolute = std::filesystem::absolute(pImpl->filePath).string();

  libzippp::ZipArchive archive(filePathAbsolute.data());

  archive.open(libzippp::ZipArchive::Write);

  archive.unlink();

  archive.close();
}

bool ArchiveDatabase::Exists() const
{
  return std::filesystem::exists(pImpl->filePath);
}
