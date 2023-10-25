#pragma once
#include <cstdint>
#include <cstddef> 

class MpForm;
class MpActor;
class WorldState;
struct VarValue;

class IPapyrusCompatibilityPolicy
{
public:
  virtual ~IPapyrusCompatibilityPolicy() = default;

  // This actor will be treated as the player character when Papyrus scripts
  // trigger legacy APIs that are obviously not designed to be used in a
  // non-singleplayer environment (like Debug.Notification)
  virtual MpActor* GetDefaultActor(const char* className, const char* funcName,
                                   int32_t stackId) const = 0;

  virtual WorldState* GetWorldState() const { return nullptr; }

  virtual void SetDefaultActor(int32_t stackId, MpActor* actor) = 0;

  virtual void BeforeSendPapyrusEvent(MpForm* form, const char* eventName,
                                      const VarValue* arguments,
                                      size_t argumentsCount,
                                      int32_t stackId) = 0;
};
