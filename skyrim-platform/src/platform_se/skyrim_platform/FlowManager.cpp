#include "FlowManager.h"

void FlowManager::CloseProcess(std::wstring pName)
{
  std::vector<DWORD> pids;

  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

  PROCESSENTRY32W entry;
  entry.dwSize = sizeof entry;

  if (!Process32FirstW(snap, &entry)) {
    return;
  }

  do {
    if (std::wstring(entry.szExeFile) == pName) {
      pids.emplace_back(entry.th32ProcessID);
    }
  } while (Process32NextW(snap, &entry));
  for (auto& process : pids) {
    HANDLE h = OpenProcess(PROCESS_TERMINATE, TRUE, process);
    TerminateProcess(h, 0);
  }
}
