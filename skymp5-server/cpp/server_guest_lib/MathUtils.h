#pragma once

namespace MathUtils {

bool IsNearlyEqual(float value, float target,
                   float margin = 1.f / 1024.f) noexcept;
float PercentToFloat(float percent) noexcept;
float PercentToMultPos(float percent) noexcept;
float PercentToMultNeg(float percent) noexcept;

}
