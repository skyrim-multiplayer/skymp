#include "JsEngine.h"
#include <spdlog/spdlog.h>

#include "NodeInstance.h"

struct JsEngine::Impl
{
  void *env = nullptr;

  std::vector<char *> argv;
  int argc = 0;

  char nodejsArgv0[5] = "node";

  std::unique_ptr<NodeInstance> nodeInstance;

  std::function<void(Napi::Env)> preparedFunction;
};

JsEngine* JsEngine::GetSingleton()
{
  static JsEngine g_instance;
  return &g_instance;
}

JsEngine::~JsEngine()
{
  spdlog::info("JsEngine::~JsEngine()");
  delete pImpl;
}

void JsEngine::AcquireEnvAndCall(const std::function<void(Napi::Env)>& f)
{  
  spdlog::info("JsEngine::AcquireEnvAndCall()");

  if (!pImpl->nodeInstance)
  {
    spdlog::error("JsEngine::AcquireEnvAndCall() - NodeInstance is nullptr");
    return;
  }

  // napi_env env = reinterpret_cast<napi_env>(pImpl->env);
  // Napi::Env cppEnv = Napi::Env(env);

  // f(cppEnv);








  // pImpl->preparedFunction = f;

  // pImpl->nodeInstance->ClearJavaScriptError();

  // int executeScriptResult = pImpl->nodeInstance->ExecuteScript(pImpl->env, R"(
  //   try {
  //     skyrimPlatformNativeAddon.callPreparedFunction();
  //   } catch (e) { 
  //     reportError(e.toString())
  //   }
  // )");

  // if (executeScriptResult != 0)
  // {
  //   spdlog::error("JsEngine::AcquireEnvAndCall() - Failed to execute script: {}", GetError());
  //   return;
  // }

  // std::string javaScriptError = pImpl->nodeInstance->GetJavaScriptError();
  // pImpl->nodeInstance->ClearJavaScriptError();

  // if (!javaScriptError.empty())
  // {
  //   spdlog::error("JsEngine::AcquireEnvAndCall() - JavaScript error: {}", javaScriptError);
  //   return;
  // }

  
  spdlog::info("JsEngine::AcquireEnvAndCall() - Leave");
}

Napi::Value JsEngine::RunScript(Napi::Env env, const std::string& src, const std::string&)
{
  spdlog::info("JsEngine::RunScript()");

  return NapiHelper::RunScript(env, src);
}

void JsEngine::ResetContext(Viet::TaskQueue<Napi::Env>&)
{
  spdlog::info("JsEngine::ResetContext()");
}

size_t JsEngine::GetMemoryUsage() const
{
  spdlog::info("JsEngine::GetMemoryUsage()");
  // TODO
  return 0;
}

JsEngine::JsEngine() : pImpl(new Impl)
{
  pImpl->argv.push_back(pImpl->nodejsArgv0);
  pImpl->argc = pImpl->argv.size();

  spdlog::info("JsEngine::JsEngine() - Enter");
  spdlog::info("JsEngine::JsEngine() - Initializing NodeInstance");

  pImpl->nodeInstance = std::make_unique<NodeInstance>();
  int initResult = pImpl->nodeInstance->Init(pImpl->argc, pImpl->argv.data());


  if (initResult != 0)
  {
    spdlog::error("JsEngine::JsEngine() - Failed to initialize NodeInstance: {}", GetError());
    pImpl->nodeInstance.reset();
    return;
  }
  
  spdlog::info("JsEngine::JsEngine() - NodeInstance initialized");

  spdlog::info("JsEngine::JsEngine() - Creating environment");

  int createEnvironmentResult = pImpl->nodeInstance->CreateEnvironment(pImpl->argc, pImpl->argv.data(), &pImpl->env);

  if (createEnvironmentResult != 0)
  {
    spdlog::error("JsEngine::JsEngine() - Failed to create environment: {}", GetError());
    pImpl->nodeInstance.reset();
    return;
  }

  spdlog::info("JsEngine::JsEngine() - Environment created");

  spdlog::info("JsEngine::JsEngine() - Executing script");

  pImpl->nodeInstance->ClearJavaScriptError();

  int executeResult = pImpl->nodeInstance->ExecuteScript(pImpl->env, R"(
    try {
      const nodeProcess = require('node:process');
      const module = { exports: {} };
      //require('./scam_native.node');
      nodeProcess.dlopen(module, 'Data/Platform/Distribution/RuntimeDependencies/SkyrimPlatformImpl.dll');

      module.exports.helloAddon();

      //globalThis.skyrimPlatformNativeAddon = module.exports;
    } catch (e) { 
      reportError(e.toString())
    }
  )");

  if (executeResult != 0)
  {
    spdlog::error("JsEngine::JsEngine() - Failed to execute script: {}", GetError());
    pImpl->nodeInstance.reset();
    return;
  }

  std::string javaScriptError = pImpl->nodeInstance->GetJavaScriptError();
  spdlog::info("JsEngine::JsEngine() - JavaScript error: {}", javaScriptError);
  pImpl->nodeInstance->ClearJavaScriptError();

  spdlog::info("JsEngine::JsEngine() - Script executed");

  spdlog::info("JsEngine::JsEngine() - Leave");
}

std::string JsEngine::GetError()
{
  size_t errorLength = pImpl->nodeInstance->GetError(nullptr, 0);
  std::vector<char> errorBuffer(errorLength);
  pImpl->nodeInstance->GetError(errorBuffer.data(), errorLength);
  std::string error(errorBuffer.data(), errorLength);
  return error;
}

#ifndef NAPI_CPP_EXCEPTIONS
#  error NAPI_CPP_EXCEPTIONS must be defined or throwing from JS code would crash!
#endif

Napi::Value CallPreparedFunction(const Napi::CallbackInfo& info)
{
  spdlog::info("CallPreparedFunction()");

  auto f = JsEngine::GetSingleton()->pImpl->preparedFunction;
  JsEngine::GetSingleton()->pImpl->preparedFunction = nullptr;

  if (f) {
    f(info.Env());
  }
  else {
    spdlog::error("CallPreparedFunction() - Prepared function is not set");
  }

  return info.Env().Undefined();
}

Napi::Value HelloAddon(const Napi::CallbackInfo& info)
{
  spdlog::info("HelloAddon()");

  return Napi::String::New(info.Env(), "Hello!");
}

Napi::Object InitNativeAddon(Napi::Env env, Napi::Object exports)
{
  spdlog::info("InitNativeAddon() - env {:x}", reinterpret_cast<uint64_t>(static_cast<napi_env>(env)));

  Napi::Number::New(env, 0);
  //exports.Set("callPreparedFunction", Napi::Number::New(env, 0));
  //exports.Set("callPreparedFunction", Napi::Function::New(env, NapiHelper::WrapCppExceptions(CallPreparedFunction)));
  exports.Set("helloAddon", Napi::Function::New(env, NapiHelper::WrapCppExceptions(HelloAddon)));
  return exports;
}

NODE_API_MODULE(skyrim_platform, InitNativeAddon)
