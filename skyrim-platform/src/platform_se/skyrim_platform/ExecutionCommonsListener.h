#pragma once
#include "CallNativeApi.h" // CallNativeApi::NativeCallRequirements
#include "JsEngine.h"
#include "SkyrimPlatformListener.h"
#include <filesystem>
#include <string>
#include <vector>

// Requires further refactoring to split into classes
class ExecutionCommonsListener : public SkyrimPlatformListener
{
public:
  ExecutionCommonsListener(
    const std::shared_ptr<CallNativeApi::NativeCallRequirements>&
      nativeCallRequirements);

  void Tick() override;
  void Update() override;
  void BeginMain() override;
  void EndMain() override;

private:
  void PerformHotReload();
  const char* GetFileDir() const;
  void LoadFiles(const std::vector<std::filesystem::path>& pathsToLoad);
  void LoadSettingsFile(const std::filesystem::path& path);
  void LoadPluginFile(const std::filesystem::path& path);
  void ClearState();
  JsEngine& GetJsEngine();
  bool EndsWith(const std::wstring& value, const std::wstring& ending);
  std::vector<std::filesystem::path> GetPathsToLoad(
    const std::filesystem::path& directory);

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};