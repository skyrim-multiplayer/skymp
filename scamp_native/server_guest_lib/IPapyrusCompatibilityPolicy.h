#pragma once

class MpActor;

class IPapyrusCompatibilityPolicy
{
public:
  virtual ~IPapyrusCompatibilityPolicy() = default;

  // This actor will be treated as the player character when Papyrus scripts
  // trigger legacy APIs that are obviously not designed to be used in a
  // non-singleplayer environment (like Debug.Notification)
  virtual MpActor* GetDefaultActor(const char* className,
                                   const char* funcName) const = 0;
};