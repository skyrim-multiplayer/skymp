#pragma once
#include "MpForm.h"
#include "papyrus-vm/VirtualMachine.h"

class MpFormGameObject : public IGameObject
{
public:
  MpFormGameObject(MpForm* form_);
  MpForm* GetFormPtr() const noexcept;

  const char* GetParentNativeScript() override;
  bool EqualsByValue(const IGameObject& obj) const override;

private:
  WorldState* const parent;
  MpForm* const form;
  const uint32_t formId;
};

template <class T>
inline T* GetFormPtr(const VarValue& papyrusObject)
{
  if (papyrusObject.GetType() != VarValue::kType_Object)
    return nullptr;
  auto gameObject = static_cast<IGameObject*>(papyrusObject);
  auto mpFormGameObject = dynamic_cast<MpFormGameObject*>(gameObject);
  if (!mpFormGameObject)
    return nullptr;
  return dynamic_cast<T*>(mpFormGameObject->GetFormPtr());
}
