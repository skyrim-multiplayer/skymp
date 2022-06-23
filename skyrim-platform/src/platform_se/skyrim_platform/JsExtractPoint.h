#pragma once

inline std::array<float, 3> JsExtractPoint(const JsValue& v)
{
  std::array<float, 3> res;

  if (v.GetType() != JsValue::Type::Array)
    throw std::runtime_error("Array expected");

  int n = (int)v.GetProperty("length");
  if (n != std::size(res))
    throw std::runtime_error("Expected array length to be " +
                             std::to_string(std::size(res)) + ", but got " +
                             std::to_string(n));

  for (int i = 0; i < n; ++i) {
    auto el = v.GetProperty(i);
    if (el.GetType() != JsValue::Type::Number)
      throw std::runtime_error(
        "Expected array element to be a number, but got '" + el.ToString() +
        "'");
    res[i] = (float)(double)el;
  }

  return res;
}
