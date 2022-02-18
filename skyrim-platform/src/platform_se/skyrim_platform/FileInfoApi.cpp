#include "FileInfoApi.h"

#include "FileInfo.h"

namespace FileInfoApi {

JsValue FileInfo(const JsFunctionArguments& args)
{
  auto path = static_cast<std::string>(args[1]);
  for (char c : path) {
    if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') ||
          ('a' <= c && c <= 'z') || c == '.' || c == '-' || c == '-')) {
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
