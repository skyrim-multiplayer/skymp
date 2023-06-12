#include "FormCallbacks.h"

FormCallbacks FormCallbacks::DoNothing() noexcept
{
  return { [](auto, auto) {}, [](auto, auto) {},
           [](auto, auto, auto, auto) {} };
}
