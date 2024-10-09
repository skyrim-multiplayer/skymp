#include <SKSE/SKSE.h>
#include <Windows.h>
#include <cstdint>

#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

#include "NodeInstance.h"

typedef bool (*SKSEPlugin_Load_Impl)(void*);

NodeInstance& GetNodeInstance()
{
  static NodeInstance g_nodeInstance;
  return g_nodeInstance;
}

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

// Initialize the Node.js engine. This function should be called before any
// other NodeJS-related function.
// Returns 0 on success, or a non-zero error code on failure.
__declspec(dllexport) int SkyrimNodeJS_Init()
{
  return GetNodeInstance().Init();
}

// Create a new Node.js runtime environment (equivalent to a new V8 Isolate).
// Returns a handle to the environment that can be used in subsequent calls.
// Returns 0 on success, or a non-zero error code on failure.
__declspec(dllexport) int SkyrimNodeJS_CreateEnvironment(int argc, char** arg,
                                                         void** outEnv)
{
  return GetNodeInstance().CreateEnvironment(argc, arg, outEnv);
}

// Destroy the Node.js runtime environment and free resources.
// This should be called when the Node.js environment is no longer needed.
// Returns 0 on success, or a non-zero error code on failure.
__declspec(dllexport) int SkyrimNodeJS_DestroyEnvironment(void* env)
{
  return GetNodeInstance().DestroyEnvironment(env);
}

// Tick the Node.js event loop. This should be called regularly to allow
// Node.js to process async events.
// Returns 0 on success, or a non-zero error code on failure.
__declspec(dllexport) int SkyrimNodeJS_Tick(void* env)
{
  return GetNodeInstance().Tick(env);
}

// Execute a JavaScript script in the Node.js environment.
// Takes the environment handle and the JavaScript code as a string.
// Returns 0 on success, or a non-zero error code on failure.
__declspec(dllexport) int SkyrimNodeJS_ExecuteScript(void* env,
                                                     const char* script)
{
  return GetNodeInstance().ExecuteScript(env, script);
}

// Get the last error message from the Node.js environment.
// Returns the length of the error message.
__declspec(dllexport) uint64_t SkyrimNodeJS_GetError(char* buffer,
                                                     uint64_t bufferSize)
{
  return GetNodeInstance().GetError(buffer, bufferSize);
}

__declspec(dllexport) bool SKSEPlugin_Load(void* skse)
{
  try {
    GetNodeInstance().Load();
    return true;
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
