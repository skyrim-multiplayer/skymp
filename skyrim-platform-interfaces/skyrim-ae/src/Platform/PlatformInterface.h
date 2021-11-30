typedef bool (*ProcessEventImpl)(void*, void*);
typedef std::function<void()> (*TickTaskImpl)();

class PlatformInterface
{
public:
  [[nodiscard]] static PlatformInterface* GetSingleton()
  {
    static PlatformInterface singleton;
    return &singleton;
  }

  bool ProcessEvent(void* event, void* args)
  {
    return processEvent(event, args);
  }

  std::function<void()> TickTask() { return tickTask(); }

private:
  PlatformInterface()
  {
    // handle path here

    HMODULE PlatformInterfaceImpl = LoadLibraryA("SkyrimPlatformImpl.dll");
    if (!PlatformInterfaceImpl) {
      logger::critical("Unable to load SkyrimPlatformImpl.dll: Error ");
    }

    processEvent = reinterpret_cast<ProcessEventImpl>(
      GetProcAddress(PlatformInterfaceImpl, "ProcessEventImpl"));
    if (!processEvent) {
      logger::critical("Unable to bind ProcessEventImpl function.");
    }

    tickTask = reinterpret_cast<TickTaskImpl>(
      GetProcAddress(PlatformInterfaceImpl, "TickTaskImpl"));
    if (!tickTask) {
      logger::critical("Unable to bind RegisterTaskImpl function.");
    }
  }

  ProcessEventImpl processEvent = nullptr;
  TickTaskImpl tickTask = nullptr;
};
