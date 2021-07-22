#pragma once
#include <fstream>
#include <functional>
#include <sstream>

#ifdef WIN32
#  include <filesystem>
namespace espm {
namespace fs = std::filesystem;
}
#else
#  include <experimental/filesystem>
namespace espm {
namespace fs = std::experimental::filesystem;
}
#endif

#include "Combiner.h"
#include "espm.h"

namespace espm {
class Loader
{
public:
  class LoadError : public std::logic_error
  {
  public:
    LoadError(std::stringstream& ss)
      : logic_error(ss.str()){};
  };

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

      if (!fs::exists(p)) {
        std::stringstream err;
        err << p.string() << " doesn't exists";
        throw LoadError(err);
      }
      std::ifstream f(p.string(), std::ios::binary);
      const auto size = fs::file_size(p);

      const clock_t was = clock();

      entry.buffer.reset(new std::vector<char>((size_t)size));
      if (f.read(entry.buffer->data(), size)) {
        entry.fileName = p.filename().string();
        entry.readDuration = float(clock() - was) / CLOCKS_PER_SEC;
        entry.size = size;
      } else {
        std::stringstream err;
        err << "Couldn't read" << p << std::endl;
        throw LoadError(err);
      }
      const clock_t was1 = clock();
      entry.browser.reset(
        new espm::Browser(entry.buffer->data(), entry.buffer->size()));
      entry.parseDuration = float(clock() - was1) / CLOCKS_PER_SEC;
      if (onProgress) {
        onProgress(entry.fileName.string(), entry.readDuration,
                   entry.parseDuration, entry.size);
      }
    }
    combiner.reset(new espm::Combiner);
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
    if (!fs::exists(dataDir)) {
      std::stringstream err;
      err << dataDir.string() << " directory doesn't exists";
      throw LoadError(err);
    }

    std::vector<fs::path> res;
    for (auto& fileName : fileNames) {
      res.push_back(dataDir / fileName);
    }
    return res;
  }

  struct Entry
  {
    std::unique_ptr<std::vector<char>> buffer;
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
}
