#pragma once
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>

namespace espm {
namespace fs = std::filesystem;
}

#include "Combiner.h"
#include "espm.h"
#include "impl/IBuffer.h"

namespace espm {

class Loader
{
public:
  using OnProgress = std::function<void(std::string fileName, float readDur,
                                        float parseDur, uintmax_t fileSize)>;

  Loader(const fs::path& dataDir, const std::vector<fs::path>& fileNames,
         OnProgress onProgress = nullptr)
    : Loader(MakeFilePaths(dataDir, fileNames), onProgress)
  {
  }

  Loader(const std::vector<fs::path>& filePaths_,
         OnProgress onProgress = nullptr)
    : filePaths(filePaths_)
  {
    for (const auto& p : filePaths) {
      entries.emplace_back();
      auto& entry = entries.back();

      const clock_t was = clock();
      entry.buffer = MakeBuffer(p);
      entry.fileName = p.filename().string();
      entry.readDuration = float(clock() - was) / CLOCKS_PER_SEC;
      entry.size = entry.buffer->GetLength();

      const clock_t was1 = clock();
      entry.browser.reset(
        new espm::Browser(entry.buffer->GetData(), entry.buffer->GetLength()));
      entry.parseDuration = float(clock() - was1) / CLOCKS_PER_SEC;
      if (onProgress) {
        onProgress(entry.fileName.string(), entry.readDuration,
                   entry.parseDuration, entry.size);
      }
    }
    combiner = std::make_unique<espm::Combiner>();
    for (auto& entry : entries) {
      const auto fileName = entry.fileName.string();
      combiner->AddSource(entry.browser.get(), fileName.c_str());
    }
    combineBrowser = combiner->Combine();
  }

  const espm::CombineBrowser& GetBrowser() const noexcept
  {
    return *combineBrowser;
  }

  std::vector<std::string> GetFileNames() const noexcept
  {
    std::vector<std::string> res;
    res.reserve(filePaths.size());
    for (auto& p : filePaths) {
      res.push_back(p.filename().string());
    }
    return res;
  }

private:
  std::vector<fs::path> MakeFilePaths(const fs::path& dataDir,
                                      const std::vector<fs::path>& fileNames)
  {
    std::vector<fs::path> res;
    for (auto& fileName : fileNames) {
      res.push_back(dataDir / fileName);
    }
    return res;
  }

  std::unique_ptr<impl::IBuffer> MakeBuffer(const fs::path& filePath) const;

  struct Entry
  {
    std::unique_ptr<impl::IBuffer> buffer;
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
};

} // namespace espm
