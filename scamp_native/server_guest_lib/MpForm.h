#pragma once
#include <string.h>
#include <cstdint>
#include <typeinfo>
#include "NiPoint3.h"

class WorldState;

class MpForm
{
  friend class WorldState;

public:
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

  template <class F>
  static const char* GetFormType()
  {
    return PrettifyType(typeid(F).name());
  }

  static const char* GetFormType(MpForm* form)
  {
    return PrettifyType(typeid(form).name());
  }

  virtual ~MpForm() = default;

  auto GetFormId() const noexcept { return formId; }

protected:
  auto GetParent() const { return parent; }

private:
  virtual void BeforeDestroy(){};

  // Assigned by WorldState::AddForm
  uint32_t formId = 0;
  WorldState* parent = nullptr;
};