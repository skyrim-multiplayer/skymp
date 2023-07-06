#include "MathUtils.h"
#include <cmath>

namespace MathUtils {

bool IsNearlyEqual(float value, float target, float margin)
{
  return std::abs(target - value) < margin;
}

}
