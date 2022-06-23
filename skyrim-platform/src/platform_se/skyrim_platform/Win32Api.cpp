#include "Win32Api.h"

JsValue Win32Api::LoadUrl(const JsFunctionArguments& args)
{
  auto str = static_cast<std::string>(args[1]);
  if (str.substr(0, 8) != "https://") {
    throw std::runtime_error(
      "Permission denied, only 'https://' prefix is allowed");
  } else {
    // CoInitializeEx is needed to call this function, but it seems we already
    // have this call somewhere in code, related link:
    // https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutea
    ShellExecute(0, 0, str.c_str(), 0, 0, SW_SHOW);
  }
  return JsValue::Undefined();
}

JsValue Win32Api::ExitProcess(const JsFunctionArguments& args)
{
  std::exit(0);
  return JsValue::Undefined();
}
