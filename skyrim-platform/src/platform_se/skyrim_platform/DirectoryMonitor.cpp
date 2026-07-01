#include "DirectoryMonitor.h"

namespace {
// How long to sleep before retrying when the watched directory is missing
// or the file-system watcher failed. Without this the watch thread burns
// 100% of a CPU core calling FindFirstChangeNotificationW in a tight loop.
constexpr DWORD kBackoffOnErrorMs = 1000;

bool WaitForNextUpdate(DWORD* outErrorCode, std::filesystem::path dir)
{
  HANDLE hDir = FindFirstChangeNotificationW(dir.c_str(), TRUE,
                                             FILE_NOTIFY_CHANGE_LAST_WRITE);
  if (hDir == INVALID_HANDLE_VALUE) {
    *outErrorCode = GetLastError();
    return false;
  }

  WaitForSingleObject(hDir, INFINITE);

  // Always release the notification handle. The old code never closed it,
  // which slowly leaked handles and could push the watch thread into a
  // high-CPU loop once the process-wide handle table filled up.
  FindCloseChangeNotification(hDir);
  return true;
}
}

struct DirectoryMonitor::Impl
{
  const std::filesystem::path dir;

  std::atomic<uint32_t> numUpdates = 0;
  std::atomic<uint32_t> errorCode = 0;
  std::atomic<bool> alive = true;

  bool thrown = false;
  uint32_t lastNumUpdates = 0;
};

DirectoryMonitor::DirectoryMonitor(std::filesystem::path dir_)
{
  pImpl.reset(new Impl{ dir_ });
  Watch();
}

DirectoryMonitor::~DirectoryMonitor()
{
  pImpl->alive = false;
}

void DirectoryMonitor::ThrowOnceIfHasError()
{
  if (pImpl->errorCode && !pImpl->thrown) {
    pImpl->thrown = true;
    if (pImpl->errorCode == 3) {
      throw std::runtime_error(
        fmt::format("Dir {} not found (it's ok, SkyrimPlatform still works)",
                    pImpl->dir.string()));
    } else {
      throw std::runtime_error(
        fmt::format("DirectoryMonitor({}) failed with code {}",
                    pImpl->dir.string(), std::to_string(pImpl->errorCode)));
    }
  }
}

bool DirectoryMonitor::Updated()
{
  uint32_t n = pImpl->numUpdates;
  bool scriptsUpdated = false;
  if (pImpl->lastNumUpdates != n) {
    pImpl->lastNumUpdates = n;
    return true;
  }
  return false;
}

void DirectoryMonitor::Watch()
{
  auto pImpl_ = pImpl;
  std::thread([pImpl_] {
    while (pImpl_->alive) {
      DWORD err = 0;
      if (WaitForNextUpdate(&err, pImpl_->dir)) {
        ++pImpl_->numUpdates;
      } else {
        pImpl_->errorCode = err;
        // Back off before retrying so we don't burn a whole CPU core
        // hammering FindFirstChangeNotificationW when the folder is
        // missing or the OS returns an error.
        Sleep(kBackoffOnErrorMs);
      }
    }
  }).detach();
}
