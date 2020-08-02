#include <Windows.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

inline void AppendToPathEnv(std::filesystem::path p)
{
  if (!p.is_absolute())
    throw std::logic_error("An absolute path expected: " + p.string());
  if (!std::filesystem::is_directory(p))
    throw std::logic_error("Expected path to be a directory: " + p.string());

  std::vector<wchar_t> path;
  path.resize(GetEnvironmentVariableW(L"PATH", nullptr, 0));
  GetEnvironmentVariableW(L"PATH", &path[0], path.size());

  std::wstring newPath = path.data();
  newPath += L';';
  newPath += p.wstring();

  if (!SetEnvironmentVariableW(L"PATH", newPath.data())) {
    throw std::runtime_error("Failed to modify PATH env: Error " +
                             std::to_string(GetLastError()));
  }
}