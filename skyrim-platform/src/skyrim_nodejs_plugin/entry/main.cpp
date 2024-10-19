#include <SKSE/SKSE.h>
#include <Windows.h>
#include <cstdint>
#include <filesystem>

#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

// skyrim-platform\src\platform_se\skyrim_platform_entry\PCH.h
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

typedef bool (*SKSEPlugin_Load_Impl)(void*);

class SkyrimNodeJsImplInterface
{
public:
  static SkyrimNodeJsImplInterface& GetSingleton()
  {
    static SkyrimNodeJsImplInterface instance;
    return instance;
  }

  bool Load(void* skse)
  {
    AppendToPathEnv(std::filesystem::current_path() / L"Data" / L"SkyrimNodeJS" /
                    L"Distribution" / L"RuntimeDependencies");

    // while (!IsDebuggerPresent()) {
    //   Sleep(1000);
    // }

    HMODULE pluginImpl = LoadLibraryA("skyrim_nodejs_dll.dll");
    if (!pluginImpl) {
      throw std::runtime_error("Unable to load skyrim_nodejs_dll.dll: Error " +
                               std::to_string(GetLastError()));
    }

    load = reinterpret_cast<SKSEPlugin_Load_Impl>(
      GetProcAddress(pluginImpl, "SkyrimNodeJS_OnSKSEPluginLoad"));
    if (!load) {
      auto err = GetLastError();
      // Get the DLL base address
      DWORD64 baseAddress = reinterpret_cast<DWORD64>(pluginImpl);

      // Initialize symbol handler
      if (!SymInitialize(GetCurrentProcess(), NULL, FALSE)) {
        throw std::runtime_error("SymInitialize failed: Error " +
                                 std::to_string(GetLastError()));
      }

      static std::string exports;

      // Enumerate symbols
      if (!SymEnumSymbols(
            GetCurrentProcess(), baseAddress, NULL,
            [](PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext) {
              exports += pSymInfo->Name;
              exports += '\n';
              return TRUE; // Continue enumeration
            },
            NULL)) {
        throw std::runtime_error("SymEnumSymbols failed: Error " +
                                 std::to_string(GetLastError()));
      }

      // Cleanup
      SymCleanup(GetCurrentProcess());

      char path[MAX_PATH];
      if (GetModuleFileNameA(pluginImpl, path, MAX_PATH) == 0) {
        throw std::runtime_error("GetModuleFileNameA failed: Error " +
                                 std::to_string(GetLastError()));
      }

      throw std::runtime_error(
        "Unable to find Foo: Error " + std::to_string(err) +
        "\nexports found:" + exports + "\npath: " + path);
    }

    return load(skse);
  }

private:
  SkyrimNodeJsImplInterface() {}

  SKSEPlugin_Load_Impl load = [](void*) { return true; };
};

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
  v.PluginName("skyrim_nodejs_dll");
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
    return SkyrimNodeJsImplInterface::GetSingleton().Load(skse);
  } catch (std::exception& e) {
    MessageBoxA(0, e.what(), "Fatal (SkyrimNodeJS)", MB_ICONERROR);
    return false;
  }
}

#ifdef SKYRIMSE
__declspec(dllexport) bool SKSEPlugin_Query(const SKSE::QueryInterface* skse,
                                            SKSE::PluginInfo* info)
{
  info->infoVersion = SKSE::PluginInfo::kVersion;
  info->name = "skyrim_nodejs_dll";
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
