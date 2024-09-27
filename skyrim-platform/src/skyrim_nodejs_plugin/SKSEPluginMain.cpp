#include <SKSE/SKSE.h>
#include <Windows.h>
#include <cstdint>

#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

#include "NodeInstance.h"

typedef bool (*SKSEPlugin_Load_Impl)(void*);

namespace Version {
inline constexpr std::size_t MAJOR = 1;
inline constexpr std::size_t MINOR = 0;
inline constexpr std::size_t PATCH = 0;
inline constexpr std::size_t BETA = 0;

inline constexpr std::uint32_t ASINT = (static_cast<std::uint32_t>(
  (MAJOR * 1000000) + (MINOR * 10000) + (PATCH * 100) + (BETA)));
}

#ifndef SKYRIMSE
constexpr SKSE::PluginVersionData GetPluginVersion()
{
  constexpr REL::Version kSPTargetRuntimeVersion1170(1, 6, 1170, 0);
  constexpr REL::Version kSPTargetRuntimeVersion640(1, 6, 640, 0);

  SKSE::PluginVersionData v;
  v.PluginVersion(Version::ASINT);
  v.PluginName("skyrim_nodejs_plugin");
  v.AuthorName("SkyMP Team");
  v.UsesAddressLibrary();
  v.UsesUpdatedStructs();
  v.CompatibleVersions(
    { kSPTargetRuntimeVersion1170, kSPTargetRuntimeVersion640 });

  return v;
};
#endif

extern "C" {

__declspec(dllexport) bool SKSEPlugin_Load(void* skse)
{
  try {
    static NodeInstance g_nodeInstance;
    return g_nodeInstance.Load();
  } catch (std::exception& e) {
    MessageBoxA(0, e.what(), "Fatal (Skyrim NodeJS Plugin)", MB_ICONERROR);
    return false;
  }
}

#ifdef SKYRIMSE
__declspec(dllexport) bool SKSEPlugin_Query(const SKSE::QueryInterface* skse,
                                            SKSE::PluginInfo* info)
{
  info->infoVersion = SKSE::PluginInfo::kVersion;
  info->name = "skyrim_nodejs_plugin";
  info->version = Version::ASINT;

  if (skse->IsEditor()) {
    //_FATALERROR("loaded in editor, marking as incompatible");
    return false;
  }
  return true;
}

#else

__declspec(dllexport) constinit SKSE::PluginVersionData SKSEPlugin_Version =
  GetPluginVersion();

#endif
}
