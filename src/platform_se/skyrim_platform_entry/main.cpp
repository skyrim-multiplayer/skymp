#include "AppendToPathEnv.h"
#include <Windows.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

typedef bool (*SKSEPlugin_Query_Impl)(void*, void*);
typedef bool (*SKSEPlugin_Load_Impl)(void*);

class PlatformImplInterface
{
public:
  static PlatformImplInterface& GetSingleton()
  {
    static PlatformImplInterface instance;
    return instance;
  }

  bool Query(void* skse, void* pluginInfo) { return query(skse, pluginInfo); }

  bool Load(void* skse) { return load(skse); }

private:
  PlatformImplInterface()
  {
    AppendToPathEnv(std::filesystem::current_path() / L"Data" / L"Platform" /
                    L"Distribution" / L"RuntimeDependencies");

    HMODULE skyrimPlatformImpl = LoadLibraryA("SkyrimPlatformImpl.dll");
    if (!skyrimPlatformImpl) {
      throw std::runtime_error(
        "Unable to load SkyrimPlatformImpl.dll: Error " +
        std::to_string(GetLastError()));
    }

    query = reinterpret_cast<SKSEPlugin_Query_Impl>(
      GetProcAddress(skyrimPlatformImpl, "SKSEPlugin_Query_Impl"));
    if (!query) {
      throw std::runtime_error("Unable to find SKSEPlugin_Query_Impl: Error " +
                               std::to_string(GetLastError()));
    }

    load = reinterpret_cast<SKSEPlugin_Load_Impl>(
      GetProcAddress(skyrimPlatformImpl, "SKSEPlugin_Load_Impl"));
    if (!load) {
      throw std::runtime_error("Unable to find SKSEPlugin_Load_Impl: Error " +
                               std::to_string(GetLastError()));
    }
  }

  SKSEPlugin_Query_Impl query = nullptr;
  SKSEPlugin_Load_Impl load = nullptr;
};

extern "C" {
__declspec(dllexport) bool SKSEPlugin_Query(void* skse, void* pluginInfo)
{
  try {
    return PlatformImplInterface::GetSingleton().Query(skse, pluginInfo);
  } catch (std::exception& e) {
    MessageBoxA(0, e.what(), "Fatal", MB_ICONERROR);
    return false;
  }
}

__declspec(dllexport) bool SKSEPlugin_Load(void* skse)
{
  try {
    return PlatformImplInterface::GetSingleton().Load(skse);
  } catch (std::exception& e) {
    MessageBoxA(0, e.what(), "Fatal", MB_ICONERROR);
    return false;
  }
}
}