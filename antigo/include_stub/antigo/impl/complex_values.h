#pragma once

#include <functional>
#include <string>

namespace Antigo::impl {

struct ReferencedValueGuard;

struct ReferencedValue {
  const ReferencedValueGuard* guard; // set nullptr on expiry
};

struct ReferencedValueGuard {
  ReferencedValue* ctxEntry;
  std::function<std::string()> printerFunc;

  void Arm() {
    if (!ctxEntry) {
      return;
    }
    ctxEntry->guard = this;
  }

  ~ReferencedValueGuard() {
    if (!ctxEntry) {
      return;
    }
    ctxEntry->guard = nullptr;
  }
};

}
