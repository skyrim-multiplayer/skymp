#include "FileInfoApi.h"
#include "FileInfo.h"
#include "InvalidArgumentException.h"
#include "Validators.h"

namespace FileInfoApi {

JsValue FileInfo(const JsFunctionArguments& args)
{
  auto filename = static_cast<std::string>(args[1]);
  if (!ValidateFilename(filename, /*allowDots*/ true)) {
    throw InvalidArgumentException("filename", filename);
  }

  const auto cppResult = ::FileInfo("Data/" + filename);
  auto jsResult = JsValue::Object();
  jsResult.SetProperty("crc32", static_cast<int>(cppResult.crc32));
  jsResult.SetProperty("size", static_cast<int>(cppResult.size));
  return jsResult;
}

}
