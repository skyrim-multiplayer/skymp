#include "libespm/Loader.h"

#include "libespm/AllocatedBuffer.h"
#include "libespm/MappedBuffer.h"

namespace espm {

Loader::Loader(const fs::path& dataDir, const std::vector<fs::path>& fileNames,
               OnProgress onProgress, BufferType bufferType_)
  : Loader(MakeFilePaths(dataDir, fileNames), onProgress, bufferType_)
{
}

Loader::Loader(const std::vector<fs::path>& filePaths_, OnProgress onProgress,
               BufferType bufferType_)
  : filePaths(filePaths_)
  , bufferType(bufferType_)
{
  for (const auto& p : filePaths) {
    entries.emplace_back();
    auto& entry = entries.back();

    const auto was = std::chrono::steady_clock::now();
    entry.buffer = MakeBuffer(p);
    entry.fileName = p.filename().string();
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<float> elapsedTime = end - was;
    entry.readDuration = elapsedTime.count();
    entry.size = entry.buffer->GetLength();

    const auto was1 = std::chrono::steady_clock::now();
    entry.browser.reset(
      new espm::Browser(entry.buffer->GetData(), entry.buffer->GetLength()));
    const auto end1 = std::chrono::steady_clock::now();
    const std::chrono::duration<float> elapsedTime1 = end1 - was1;
    entry.parseDuration = elapsedTime1.count();
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

const espm::CombineBrowser& Loader::GetBrowser() const noexcept
{
  return *combineBrowser;
}

std::vector<std::string> Loader::GetFileNames() const noexcept
{
  std::vector<std::string> res;
  res.reserve(filePaths.size());
  for (auto& p : filePaths) {
    res.push_back(p.filename().string());
  }
  return res;
}

std::map<std::string, Loader::FileInfo> Loader::GetFilesInfo() const
{
  std::map<std::string, FileInfo> res;

  for (const auto& entry : entries) {
    auto hash =
      CalculateHashcode(entry.buffer->GetData(), entry.buffer->GetLength());
    res.emplace(entry.fileName.string(),
                FileInfo{ hash, entry.buffer->GetLength() });
  }

  return res;
}

std::vector<fs::path> Loader::MakeFilePaths(
  const fs::path& dataDir, const std::vector<fs::path>& fileNames)
{
  std::vector<fs::path> res;
  for (auto& fileName : fileNames) {
    res.push_back(dataDir / fileName);
  }
  return res;
}

std::unique_ptr<IBuffer> Loader::MakeBuffer(const fs::path& filePath) const
{
  switch (bufferType) {
    case BufferType::AllocatedBuffer:
      return std::make_unique<AllocatedBuffer>(filePath);
    case BufferType::MappedBuffer:
      return std::make_unique<MappedBuffer>(filePath);
    default:
      throw std::runtime_error("[espm] unhandled buffer type");
  }
}

} // namespace espm
