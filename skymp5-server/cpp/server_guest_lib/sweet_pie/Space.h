#include "NiPoint3.h"
#include <functional>

namespace sweetpie {
struct Space
{
  NiPoint3 position;
  std::function<bool(const NiPoint3&)> IsInside =
    [](const NiPoint3& newPosition) { return true; };
};
}
