#include "MpFormGameObject.h"

#include "WorldState.h"

MpFormGameObject::MpFormGameObject(MpForm* form_)
  : form(form_)
  , parent(form_ ? form_->GetParent() : nullptr)
  , formId(form_ ? form_->GetFormId() : 0)
{
}

MpForm* MpFormGameObject::GetFormPtr() const noexcept
{
  if (parent) {
    bool formStillValid = parent->LookupFormById(formId).get() == form;
    if (!formStillValid)
      return nullptr;
  }
  return form;
}

const char* MpFormGameObject::GetParentNativeScript()
{
  if (auto form = GetFormPtr())
    return form->GetFormType();
  return "";
}