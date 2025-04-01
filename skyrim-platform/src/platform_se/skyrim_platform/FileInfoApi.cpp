#include "FileInfoApi.h"
#include "FileInfo.h"
#include "InvalidArgumentException.h"
#include "Validators.h"

namespace FileInfoApi {

Napi::Value FileInfo(const Napi::CallbackInfo& info)
{
  auto filename = NapiHelper::ExtractString(info[0], "filename");

  if (!ValidateFilename(filename, /*allowDots*/ true)) {
    throw InvalidArgumentException("filename", filename);
  }

  const auto cppResult = ::FileInfo("Data/" + filename);

  // javascript code expects int32_t version of CRC32, not uint32_t
  int src32signed = static_cast<int>(cppResult.crc32);

  auto jsResult = Napi::Object::New(info.Env());
  jsResult.Set("crc32", Napi::Number::New(info.Env(), src32signed));
  jsResult.Set("size", Napi::Number::New(info.Env(), cppResult.size));
  return jsResult;
}

}
