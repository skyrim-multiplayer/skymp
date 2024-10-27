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

  try {
    int executeResult = pImpl->nodeInstance->ExecuteScript(pImpl->env, "try { require('node:process').dlopen('Data/Platform/Distribution/RuntimeDependencies/SkyrimPlatformImpl.dll'); } catch (e) {}");

    if (executeResult != 0)
    {
      spdlog::error("JsEngine::JsEngine() - Failed to execute script: {}", GetError());
      pImpl->nodeInstance.reset();
      return;
    }
  }
  catch (const std::exception& e)
  {
    spdlog::error("JsEngine::JsEngine() - Script exception: {}", e.what());
    pImpl->nodeInstance.reset();
    return;
  }

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

Napi::Object InitNativeAddon(Napi::Env env, Napi::Object exports)
{
  spdlog::info("InitNativeAddon()");
  return exports;
}

NODE_API_MODULE(skyrim_platform, InitNativeAddon)
