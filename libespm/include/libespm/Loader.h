#pragma once
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <sstream>

namespace espm {
namespace fs = std::filesystem;
}

#include "Browser.h"
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

  const CombineBrowser& GetBrowser() const noexcept;

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

  std::unique_ptr<Viet::IBuffer> MakeBuffer(const fs::path& filePath) const;

  struct Entry
  {
    std::unique_ptr<Viet::IBuffer> buffer;
    std::unique_ptr<Browser> browser;

    uintmax_t size = 0;
    fs::path fileName = "";
    float readDuration = 0;
    float parseDuration = 0;
  };

  std::vector<Entry> entries;
  std::unique_ptr<Combiner> combiner;
  std::unique_ptr<CombineBrowser> combineBrowser;
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

  const LookupResult lookupResult = espmLoader.GetBrowser().LookupById(formId);

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
  CompressedFieldsCache& espmCache = espmProvider->GetEspmCache();

  const LookupResult lookupResult = espmLoader.GetBrowser().LookupById(formId);

  if (!lookupResult.rec) {
    throw std::runtime_error(
      fmt::format("Record {:#x} doesn't exist", formId));
  }

  const RecordT* convertedRecord = Convert<RecordT>(lookupResult.rec);
  if (!convertedRecord) {
    throw std::runtime_error(
      fmt::format("Expected record {:#x} to be {}, but found {}", formId,
                  RecordT::kType, lookupResult.rec->GetType().ToString()));
  }

  return convertedRecord->GetData(espmCache);
}

} // namespace espm
