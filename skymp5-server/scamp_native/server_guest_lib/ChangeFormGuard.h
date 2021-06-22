#pragma once
#include "MpObjectReference.h"
#include <chrono>
#include <optional>

namespace ChangeFormGuard_ {
void RequestSave(MpObjectReference* self);
}

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
    if (!blockSaving && mode == Mode::RequestSave) {
      lastSaveRequest = std::chrono::system_clock::now();
      ChangeFormGuard_::RequestSave(self);
    }
  }

  const T& ChangeForm() const noexcept { return changeForm; }

  auto GetLastSaveRequestMoment() const { return lastSaveRequest; }

  bool blockSaving = false;

private:
  T changeForm;
  MpObjectReference* const self;
  std::optional<std::chrono::system_clock::time_point> lastSaveRequest;
};