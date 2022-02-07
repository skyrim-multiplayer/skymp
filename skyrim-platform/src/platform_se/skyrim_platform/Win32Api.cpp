#include "Win32Api.h"

#include <stdexcept>
#include <filesystem>
#include <vector>

#include <zlib.h>

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

JsValue FileInfo(const JsFunctionArguments& args) {
  auto path = static_cast<std::string>(args[1]);
  for (char c : path) {
    if (!(('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || c == '.')) {
      throw std::runtime_error("FileInfo: forbidden characters in path");
    }
  }

  path = "Data/" + path;

  size_t size = std::filesystem::file_size(path);
  std::vector<char> buf(size);

  std::ifstream f(path, std::ios::binary);
  if (!f.read(buf.data(), size)) {
    throw std::runtime_error("FileInfo: can't read " + path);
  }

  auto hash = crc32_z(0L, Z_NULL, 0);
  hash = crc32_z(hash, reinterpret_cast<const Bytef*>(buf.data()), size);

  auto result = JsValue::Object();
  result.SetProperty("crc32", static_cast<int>(hash));
  result.SetProperty("size", static_cast<int>(size));
  return result;
}

}
