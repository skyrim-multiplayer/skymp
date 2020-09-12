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

  Loader(const fs::path& dataDir, const std::vector<fs::path>& files_,
         OnProgress onProgress = nullptr)
    : files(files_)
  {
    std::stringstream err;
    if (!fs::exists(dataDir)) {
      err << dataDir.string() << " directory doesn't exists";
      throw LoadError(err);
    }

    for (const auto& file : files) {
      entries.push_back(Entry());
      auto& entry = entries.back();

      const fs::path p = dataDir / file;
      if (!fs::exists(p)) {
        err << p.string() << " doesn't exists";
        throw LoadError(err);
      }
      std::ifstream f(p.string(), std::ios::binary);
      const auto size = fs::file_size(p);

      const clock_t was = clock();

      entry.buffer.reset(new std::vector<char>((size_t)size));
      if (f.read(entry.buffer->data(), size)) {
        entry.fileName = file;
        entry.readDuration = float(clock() - was) / CLOCKS_PER_SEC;
        entry.size = size;
      } else {
        err << "Couldn't read" << std::endl;
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

  const std::vector<fs::path>& GetFileNames() const noexcept { return files; }

private:
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
  const std::vector<fs::path> files;
};
}