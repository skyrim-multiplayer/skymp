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
  auto jsResult = Napi::Object::New(info.Env());
  jsResult.Set("crc32", Napi::Number::New(info.Env(), cppResult.crc32));
  jsResult.Set("size", Napi::Number::New(info.Env(), cppResult.size));
  return jsResult;
}

}
