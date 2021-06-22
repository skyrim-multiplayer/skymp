#pragma once
#include <Windows.h>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <thread>

class DirectoryMonitor
{
public:
  DirectoryMonitor(std::filesystem::path dir_);
  ~DirectoryMonitor();
  void ThrowOnceIfHasError();
  bool Updated();

private:
  void Watch();

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};