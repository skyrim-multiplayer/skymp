#include "MathUtils.h"
#include <cmath>

namespace MathUtils {

bool IsNearlyEqual(float value, float target, float margin) noexcept
{
  return std::abs(target - value) < margin;
}

float PercentToFloat(float percent) noexcept
{
  return percent / 100.f;
}

float PercentToMultPos(float percent) noexcept
{
  return 1.f + PercentToFloat(percent);
}

float PercentToMultNeg(float percent) noexcept
{
  return 1.f - PercentToFloat(percent);
}

}
