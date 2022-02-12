#include "Win32Api.h"

#include "FileInfo.h"

namespace Win32Api {

JsValue LoadUrl(const JsFunctionArguments& args)
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

JsValue ExitProcess(const JsFunctionArguments& args)
{
  std::exit(0);
  return JsValue::Undefined();
}

JsValue FileInfo(const JsFunctionArguments& args)
{
  auto path = static_cast<std::string>(args[1]);
  for (char c : path) {
    if (!(('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || c == '.')) {
      throw std::runtime_error("FileInfo: forbidden characters in path");
    }
  }

  const auto cppResult = ::FileInfo("Data/" + path);
  auto jsResult = JsValue::Object();
  jsResult.SetProperty("crc32", static_cast<int>(cppResult.crc32));
  jsResult.SetProperty("size", static_cast<int>(cppResult.size));
  return jsResult;
}

}
