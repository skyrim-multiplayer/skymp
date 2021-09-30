#pragma once
#include <include/cef_values.h>
#include <string>

class ProcessMessageListener
{
public:
  virtual ~ProcessMessageListener() = default;

  virtual void OnProcessMessage(
    const std::string& name,
    const CefRefPtr<CefListValue>& arguments) noexcept = 0;
};
