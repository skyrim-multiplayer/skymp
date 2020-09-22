#pragma once
#include "MpObjectReference.h"

template <class T>
class ChangeFormGuard
{
public:
  ChangeFormGuard(T changeForm_, MpObjectReference* self_)
    : changeForm(changeForm_)
    , self(self_)
  {
  }

  enum class Mode
  {
    RequestSave,
    NoRequestSave
  };

  template <class F>
  void EditChangeForm(F f, Mode mode = Mode::RequestSave)
  {
    f(changeForm);
    if (mode == Mode::RequestSave)
      self->GetParent()->RequestSave(*self);
  }

  const T& ChangeForm() const noexcept { return changeForm; }

private:
  T changeForm;
  MpObjectReference* const self;
};