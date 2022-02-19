#pragma once
#include "JsEngine.h"

namespace FileInfoApi {

JsValue FileInfo(const JsFunctionArguments& args);

inline void Register(JsValue& exports)
{
  exports.SetProperty("getFileInfo", JsValue::Function(FileInfo));
}

}
