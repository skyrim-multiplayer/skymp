#pragma once

/* disable unused headers from Windows.h */
#define WIN32_LEAN_AND_MEAN
#define NOSOUND          // Sound driver routines
#define NOTEXTMETRIC     // typedef TEXTMETRIC and associated routines
#define NOWH             // SetWindowsHook and WH_*
#define NOWINOFFSETS     // GWL_*, GCL_*, associated routines
#define NOCOMM           // COMM driver routines
#define NOKANJI          // Kanji support stuff.
#define NOHELP           // Help engine interface.
#define NOPROFILER       // Profiler interface.
#define NODEFERWINDOWPOS // DeferWindowPos routines
#define NOMCX            // Modem Configuration Extensions
#define NOGDICAPMASKS    // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOSYSMETRICS     // SM_*
#define NOMENUS          // MF_*
#define NOICONS          // IDI_*
#define NOKEYSTATES      // MK_*
#define NOSYSCOMMANDS    // SC_*
#define NORASTEROPS      // Binary and Tertiary raster ops
#define NOSHOWWINDOW     // SW_*
#define OEMRESOURCE      // OEM Resource values
#define NOATOM           // Atom Manager routines
#define NOCLIPBOARD      // Clipboard routines
#define NOCOLOR          // Screen colors
#define NODRAWTEXT       // DrawText() and DT_*
#define NOGDI            // All GDI defines and routines
#define NOKERNEL         // All KERNEL defines and routines
#define NONLS            // All NLS defines and routines
#define NOMEMMGR         // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE       // typedef METAFILEPICT
#define NOMINMAX         // Macros min(a,b) and max(a,b)
#define NOOPENFILE       // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL         // SB_* and scrolling routines
#define NOSERVICE // All Service Controller routines, SERVICE_ equates, etc.

#include <Windows.h>
#include <cassert>
#include <filesystem>
#include <span>

using namespace std::literals;

#define DLLEXPORT __declspec(dllexport)

#include "Version.h"

#define MAKE_EXE_VERSION_EX(major, minor, build, sub)                         \
  ((((major)&0xFF) << 24) | (((minor)&0xFF) << 16) | (((build)&0xFFF) << 4) | \
   ((sub)&0xF))
#define MAKE_EXE_VERSION(major, minor, build)                                 \
  MAKE_EXE_VERSION_EX(major, minor, build, 0)

#define RUNTIME_VERSION_1_6_317 MAKE_EXE_VERSION(1, 6, 317)
#define RUNTIME_VERSION_1_6_318 MAKE_EXE_VERSION(1, 6, 318)
#define RUNTIME_VERSION_1_6_323 MAKE_EXE_VERSION(1, 6, 323)
#define RUNTIME_VERSION_1_6_342 MAKE_EXE_VERSION(1, 6, 323)

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

struct PluginVersionData
{
public:
  enum
  {
    kVersion = 1,
  };

  const std::uint32_t dataVersion{ kVersion };
  std::uint32_t pluginVersion = 0;
  char pluginName[256] = {};
  char author[256] = {};
  char supportEmail[256] = {};
  bool addressLibrary : 1 = false;
  bool sigScanning : 1 = false;
  std::uint8_t padding1 : 6 = 0;
  std::uint8_t padding2 = 0;
  std::uint16_t padding3 = 0;
  std::uint32_t compatibleVersions[16] = { RUNTIME_VERSION_1_6_342,
                                           RUNTIME_VERSION_1_6_323,
                                           RUNTIME_VERSION_1_6_318,
                                           RUNTIME_VERSION_1_6_317,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0 };
  std::uint32_t xseMinimum = 0;
};

static constexpr void SetCharBuffer(std::string_view a_src,
                                    std::span<char> a_dst) noexcept
{
  assert(a_src.size() < a_dst.size());
  std::fill(a_dst.begin(), a_dst.end(), '\0');
  std::copy(a_src.begin(), a_src.end(), a_dst.begin());
}
