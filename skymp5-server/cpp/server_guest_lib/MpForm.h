#pragma once
#include "NiPoint3.h"
#include "papyrus-vm/Structures.h"
#include <cstdint>
#include <memory>
#include <string.h>
#include <typeinfo>

class WorldState;
class IGameObject;

class MpForm
{
  friend class WorldState;

public:
  MpForm();

  static const char* Type() { return "Form"; }
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

protected:
  virtual void BeforeDestroy(){};
};
