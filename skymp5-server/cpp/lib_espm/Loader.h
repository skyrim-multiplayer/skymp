#pragma once
#include <filesystem>
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

  std::map<std::string, uint32_t> GetHashes() const;

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

} // namespace espm
