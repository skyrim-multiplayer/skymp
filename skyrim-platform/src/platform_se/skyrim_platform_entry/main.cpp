typedef void (*IpcMessageCallback)(const uint8_t* data, uint32_t length,
                                   void* state);

typedef bool (*SKSEPlugin_Load_Impl)(void*);
typedef uint32_t (*SkyrimPlatform_IpcSubscribe_Impl)(const char*,
                                                     IpcMessageCallback,
                                                     void*);
typedef void (*SkyrimPlatform_IpcUnsubscribe_Impl)(uint32_t);
typedef void (*SkyrimPlatform_IpcSend_Impl)(const char*, const uint8_t*,
                                            uint32_t);

class PlatformImplInterface
{
public:
  static PlatformImplInterface& GetSingleton()
  {
    static PlatformImplInterface instance;
    return instance;
  }

  bool Load(void* skse) { return load(skse); }

  uint32_t IpcSubscribe(const char* systemName, IpcMessageCallback callback,
                        void* state)
  {
    return ipcSubscribe(systemName, callback, state);
  }

  void IpcUnsubscribe(uint32_t subscriptionId)
  {
    return ipcUnsubscribe(subscriptionId);
  }

  void IpcSend(const char* systemName, const uint8_t* data, uint32_t length)
  {
    return ipcSend(systemName, data, length);
  }

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

    load = reinterpret_cast<SKSEPlugin_Load_Impl>(
      GetProcAddress(skyrimPlatformImpl, "SKSEPlugin_Load_Impl"));
    if (!load) {
      throw std::runtime_error("Unable to find SKSEPlugin_Load_Impl: Error " +
                               std::to_string(GetLastError()));
    }

    ipcSubscribe = reinterpret_cast<SkyrimPlatform_IpcSubscribe_Impl>(
      GetProcAddress(skyrimPlatformImpl, "SkyrimPlatform_IpcSubscribe_Impl"));
    if (!ipcSubscribe) {
      throw std::runtime_error(
        "Unable to find SkyrimPlatform_IpcSubscribe_Impl: Error " +
        std::to_string(GetLastError()));
    }

    ipcSend = reinterpret_cast<SkyrimPlatform_IpcSend_Impl>(
      GetProcAddress(skyrimPlatformImpl, "SkyrimPlatform_IpcSend_Impl"));
    if (!ipcSend) {
      throw std::runtime_error(
        "Unable to find SkyrimPlatform_IpcSend_Impl: Error " +
        std::to_string(GetLastError()));
    }

    ipcUnsubscribe =
      reinterpret_cast<SkyrimPlatform_IpcUnsubscribe_Impl>(GetProcAddress(
        skyrimPlatformImpl, "SkyrimPlatform_IpcUnsubscribe_Impl"));
    if (!ipcUnsubscribe) {
      throw std::runtime_error(
        "Unable to find SkyrimPlatform_IpcUnsubscribe_Impl: Error " +
        std::to_string(GetLastError()));
    }
  }

  SKSEPlugin_Load_Impl load = nullptr;
  SkyrimPlatform_IpcSubscribe_Impl ipcSubscribe = nullptr;
  SkyrimPlatform_IpcSend_Impl ipcSend = nullptr;
  SkyrimPlatform_IpcUnsubscribe_Impl ipcUnsubscribe = nullptr;
};

extern "C" {

#ifdef SKYRIMSE
DLLEXPORT bool SKSEPlugin_Query(const SKSE::QueryInterface* skse,
                                SKSE::PluginInfo* info)
{
  info->infoVersion = SKSE::PluginInfo::kVersion;
  info->name = "SkyrimPlatform";
  info->version = Version::ASINT;

  if (skse->IsEditor()) {
    //_FATALERROR("loaded in editor, marking as incompatible");
    return false;
  }
  return true;
}

#else
DLLEXPORT constinit auto SKSEPlugin_Version = []() {
  SKSE::PluginVersionData v;
  v.PluginVersion(Version::ASINT);
  v.PluginName("SkyrimPlatform");
  v.AuthorName("SkyMP Team and Contributors");
  v.UsesAddressLibrary(true);
  v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

  return v;
}();

#endif

DLLEXPORT uint32_t SkyrimPlatform_IpcSubscribe(const char* systemName,
                                               IpcMessageCallback callback,
                                               void* state)
{
  return PlatformImplInterface::GetSingleton().IpcSubscribe(systemName,
                                                            callback, state);
}

DLLEXPORT void SkyrimPlatform_IpcUnsubscribe(uint32_t subscriptionId)
{
  return PlatformImplInterface::GetSingleton().IpcUnsubscribe(subscriptionId);
}

DLLEXPORT void SkyrimPlatform_IpcSend(const char* systemName,
                                      const uint8_t* data, uint32_t length)
{
  return PlatformImplInterface::GetSingleton().IpcSend(systemName, data,
                                                       length);
}

DLLEXPORT bool SKSEPlugin_Load(void* skse)
{
  try {
    return PlatformImplInterface::GetSingleton().Load(skse);
  } catch (std::exception& e) {
    MessageBoxA(0, e.what(), "Fatal", MB_ICONERROR);
    return false;
  }
}
}
