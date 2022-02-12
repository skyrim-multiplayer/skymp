#pragma once
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>

namespace espm {
namespace fs = std::filesystem;
}

#include "Combiner.h"
#include "IBuffer.h"
#include "espm.h"

namespace espm {

class Loader
{
public:
  enum class BufferType
  {
    AllocatedBuffer,
    MappedBuffer,
  };

  using OnProgress = std::function<void(std::string fileName, float readDur,
                                        float parseDur, uintmax_t fileSize)>;

  Loader(const fs::path& dataDir, const std::vector<fs::path>& fileNames,
         OnProgress onProgress = nullptr,
         BufferType bufferType_ = BufferType::MappedBuffer);

  Loader(const std::vector<fs::path>& filePaths_,
         OnProgress onProgress = nullptr,
         BufferType bufferType_ = BufferType::MappedBuffer);

  const espm::CombineBrowser& GetBrowser() const noexcept;

  std::vector<std::string> GetFileNames() const noexcept;

  struct FileInfo
  {
    uint32_t crc32 = 0;
    size_t size = 0;
  };

  std::map<std::string, FileInfo> GetFilesInfo() const;

private:
  std::vector<fs::path> MakeFilePaths(const fs::path& dataDir,
                                      const std::vector<fs::path>& fileNames);

  std::unique_ptr<IBuffer> MakeBuffer(const fs::path& filePath) const;

  struct Entry
  {
    std::unique_ptr<IBuffer> buffer;
    std::unique_ptr<espm::Browser> browser;

    uintmax_t size = 0;
    fs::path fileName = "";
    float readDuration = 0;
    float parseDuration = 0;
  };

  std::vector<Entry> entries;
  std::unique_ptr<espm::Combiner> combiner;
  std::unique_ptr<espm::CombineBrowser> combineBrowser;
  std::vector<fs::path> filePaths;
  BufferType bufferType;
};

template <class EspmProvider>
Type GetRecordType(uint32_t formId, EspmProvider* espmProvider)
{
  if (!espmProvider) {
    throw std::runtime_error("Unable to find record without EspmProvider");
  }

  auto& espmLoader = espmProvider->GetEspm();

  const espm::LookupResult lookupResult =
    espmLoader.GetBrowser().LookupById(formId);

  if (!lookupResult.rec) {
    throw std::runtime_error(
      fmt::format("Record {0:x} doesn't exist", formId));
  }

  return lookupResult.rec->GetType();
}

template <class RecordT, class EspmProvider>
typename RecordT::Data GetData(uint32_t formId, EspmProvider* espmProvider)
{
  if (!espmProvider) {
    throw std::runtime_error("Unable to find record without EspmProvider");
  }

  auto& espmLoader = espmProvider->GetEspm();

  // That's why espmProvider is non-const: cache is mutable
  espm::CompressedFieldsCache& espmCache = espmProvider->GetEspmCache();

  const espm::LookupResult lookupResult =
    espmLoader.GetBrowser().LookupById(formId);

  if (!lookupResult.rec) {
    throw std::runtime_error(
      fmt::format("Record {:#x} doesn't exist", formId));
  }

  const RecordT* convertedRecord = espm::Convert<RecordT>(lookupResult.rec);
  if (!convertedRecord) {
    throw std::runtime_error(
      fmt::format("Expected record {:#x} to be {}, but found {}", formId,
                  RecordT::kType, lookupResult.rec->GetType().ToString()));
  }

  return convertedRecord->GetData(espmCache);
}

} // namespace espm
