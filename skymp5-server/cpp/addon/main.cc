#include "AsyncSaveStorage.h"
#include "EspmGameObject.h"
#include "FileDatabase.h"
#include "FormCallbacks.h"
#include "GamemodeApi.h"
#include "LocalizationProvider.h"
#include "MigrationDatabase.h"
#include "MongoDatabase.h"
#include "MpFormGameObject.h"
#include "Networking.h"
#include "NetworkingCombined.h"
#include "NetworkingMock.h"
#include "PartOne.h"
#include "ScriptStorage.h"
#include "formulas/TES5DamageFormula.h"
#include <JsEngine.h>
#include <cassert>
#include <cctype>
#include <memory>
#include <napi.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "ScampServer.h"

#ifndef NAPI_CPP_EXCEPTIONS
#  error NAPI_CPP_EXCEPTIONS must be defined or throwing from JS code would crash!
#endif

namespace {


std::shared_ptr<ISaveStorage> CreateSaveStorage(
  std::shared_ptr<IDatabase> db, std::shared_ptr<spdlog::logger> logger)
{
  return std::make_shared<AsyncSaveStorage>(db, logger);
}

static std::shared_ptr<spdlog::logger>& GetLogger()
{
  static auto g_logger = spdlog::stdout_color_mt("console");
  return g_logger;
}

}

std::string GetPropertyAlphabet()
{
  std::string alphabet;
  for (char c = 'a'; c <= 'z'; c++) {
    alphabet += c;
  }
  for (char c = 'A'; c <= 'Z'; c++) {
    alphabet += c;
  }
  for (char c = '0'; c <= '9'; c++) {
    alphabet += c;
  }
  alphabet += '_';
  return alphabet;
}

Napi::Value ScampServer::Get(const Napi::CallbackInfo &info) {
  try {
    auto formId = NapiHelper::ExtractUInt32(info[0], "formId");
    auto propertyName = NapiHelper::ExtractString(info[1], "propertyName");
  }
  catch (std::exception &e) {
    throw Napi::Error::New(info.Env(), std::string(e.what()));
  }
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  return ScampServer::Init(env, exports);
}

NODE_API_MODULE(scamp, Init)
