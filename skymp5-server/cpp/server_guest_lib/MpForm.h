#pragma once
#include "NiPoint3.h"
#include <cstdint>
#include <memory>
#include <string.h>
#include <typeinfo>
#include <vector>

class WorldState;
class IGameObject;
class ActivePexInstance;
struct VarValue;

class MpForm
{
  friend class WorldState;
  friend class MpFormGameObject;

public:
  MpForm();

  static const char* Type() { return "Form"; }
  bool IsEspmForm() const noexcept;
  virtual const char* GetFormType() const { return "Form"; }

  auto GetParent() const { return parent; }

  static const char* PrettifyType(const char* typeidName)
  {
#ifdef WIN32
    return typeidName + strlen("class Mp");
#else
    auto name = typeidName;
    while (memcmp(name, "Mp", 2) != 0)
      ++name;
    return name + 2;
#endif
  }

  VarValue ToVarValue() const;
  std::shared_ptr<IGameObject> ToGameObject() const;

  template <class F>
  static const char* GetFormType()
  {
    return PrettifyType(typeid(F).name());
  }

  static const char* GetFormType(MpForm* form)
  {
    return PrettifyType(typeid(*form).name());
  }

  virtual ~MpForm() = default;

  auto GetFormId() const noexcept { return id; }

  MpForm(const MpForm&) = delete;
  MpForm& operator=(const MpForm&) = delete;

protected:
  virtual void Init(WorldState* parent_, uint32_t formId_,
                    bool hasChangeForm); // See WorldState::AddForm
  virtual void Update();
  virtual void SendPapyrusEvent(const char* eventName,
                                const VarValue* arguments = nullptr,
                                size_t argumentsCount = 0);

private:
  using GameObjectPtr = std::shared_ptr<IGameObject>;

  uint32_t id = 0;
  WorldState* parent = nullptr;
  mutable GameObjectPtr gameObject;
  std::vector<std::shared_ptr<ActivePexInstance>> activePexInstances;

protected:
  virtual void BeforeDestroy(){};

  const std::vector<std::shared_ptr<ActivePexInstance>>&
  ListActivePexInstances() const;

  void AddScript(const std::shared_ptr<ActivePexInstance>& script) noexcept;
};
