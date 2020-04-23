#pragma once
#include <Windows.h>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <thread>

class DirectoryMonitor
{
public:
  DirectoryMonitor(std::filesystem::path dir_)
    : dir(dir_)
  {
    Watch();
  }

  ~DirectoryMonitor() = delete;

  uint32_t GetNumUpdates() const noexcept { return numUpdates; }

  DWORD GetErrorCode() const { return errorCode; }

private:
  void Watch()
  {
    std::thread([this] {
      while (true) {
        DWORD err = 0;
        if (WaitForNextUpdate(&err))
          ++numUpdates;
        else
          errorCode = err;
      }
    }).detach();
  }

  bool WaitForNextUpdate(DWORD* outErrorCode)
  {
    HANDLE hDir;
    hDir = FindFirstChangeNotificationW(dir.c_str(), TRUE,
                                        FILE_NOTIFY_CHANGE_LAST_WRITE);
    if (hDir == INVALID_HANDLE_VALUE) {
      *outErrorCode = GetLastError();
      return false;
    }
    WaitForSingleObject(hDir, INFINITE);
    return true;
  }

  std::atomic<uint32_t> numUpdates = 0;
  std::atomic<uint32_t> errorCode = 0;
  const std::filesystem::path dir;
};