#pragma once

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
