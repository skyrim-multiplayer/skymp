#pragma once
#include <memory>
#include <vector>

class VirtualMachine;
class ActivePexInstance;

class IGameObject
{
  friend class VirtualMachine;
  friend class ActivePexInstance;

public:
  virtual ~IGameObject() = default;
  virtual const char* GetStringID() { return "Virtual Implementation"; };

  // 'Actor', 'ObjectReference' and so on. Used for dynamic casts
  virtual const char* GetParentNativeScript() { return ""; }

  virtual bool EqualsByValue(const IGameObject& obj) const { return false; }

  bool HasScript(const char* name) const;

protected:
  virtual const std::vector<std::shared_ptr<ActivePexInstance>>&
  ListActivePexInstances() const = 0;

  virtual void AddScript(
    std::shared_ptr<ActivePexInstance> sctipt) noexcept = 0;
};