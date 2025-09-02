#include "JsEngine.h"
#include <spdlog/spdlog.h>
#include <stack>

#include "NodeInstance.h"
#include "ScopedTask.h"

namespace {
enum V8ScriptId : uint16_t
{
  V8SCRIPT_INIT_SKYRIM_PLATFORM,
  V8SCRIPT_CALL_PREPARED_FUNCTION,
};

constexpr auto kInitSkyrimPlatformScript = R"(
    try {
      const nodeProcess = require('node:process');
      const module = { exports: {} };

      nodeProcess.dlopen(module, 'Data/Platform/Distribution/RuntimeDependencies/SkyrimPlatformImpl.dll');

      globalThis.skyrimPlatformNativeAddon = module.exports;
    } catch (e) { 
      reportError(e.toString());
    }
  )";

constexpr auto kCallPreparedFunctionSсript = R"(
    try {
      skyrimPlatformNativeAddon.callPreparedFunction();
    } catch (e) { 
      reportError(e.toString());
    }
  )";
}

struct JsEngine::Impl
{
  void* env = nullptr;

  std::vector<char*> argv;
  int argc = 0;
  char nodejsArgv0[5] = "node";

  std::unique_ptr<NodeInstance> nodeInstance;
  std::stack<std::function<void(Napi::Env)>> preparedFunctionsStack;
  int depth = 0;
  std::optional<Napi::Env> retrievedEnv;

  std::recursive_mutex m;
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

void JsEngine::AcquireEnvAndCall(const std::function<void(Napi::Env)>& f,
                                 const char* comment)
{
  std::lock_guard l(pImpl->m);

  Viet::ScopedTask<int> task([](int& depth) { depth--; }, pImpl->depth);
  pImpl->depth++;

  Viet::ScopedTask<std::stack<std::function<void(Napi::Env)>>> task2(
    [](auto& preparedFunctionsStack) { preparedFunctionsStack.pop(); },
    pImpl->preparedFunctionsStack);
  pImpl->preparedFunctionsStack.push(f);

  if (pImpl->depth > 1) {
    return f(*pImpl->retrievedEnv);
  }

  if (!pImpl->nodeInstance) {
    spdlog::error("JsEngine::AcquireEnvAndCall() - NodeInstance is nullptr");
    return;
  }

  pImpl->nodeInstance->ClearJavaScriptError();

  int executeScriptResult = pImpl->nodeInstance->ExecuteScript(
    pImpl->env, V8SCRIPT_CALL_PREPARED_FUNCTION);

  if (executeScriptResult != 0) {
    spdlog::error(
      "JsEngine::AcquireEnvAndCall() - Failed to execute script: {}",
      GetError());
    return;
  }

  std::string javaScriptError = pImpl->nodeInstance->GetJavaScriptError();
  pImpl->nodeInstance->ClearJavaScriptError();

  if (!javaScriptError.empty()) {
    spdlog::error(
      "JsEngine::AcquireEnvAndCall() - Rethrowing JavaScript error: {}",
      javaScriptError);
    throw std::runtime_error(javaScriptError);
  }
}

Napi::Value JsEngine::RunScript(Napi::Env env, const std::string& src,
                                const std::string&)
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

void JsEngine::Tick()
{
  if (!pImpl->nodeInstance) {
    spdlog::error("JsEngine::Tick() - NodeInstance is nullptr");
    return;
  }

  if (!pImpl->env) {
    spdlog::error("JsEngine::Tick() - Environment is nullptr");
    return;
  }

  pImpl->nodeInstance->Tick(pImpl->env);
}

JsEngine::JsEngine()
  : pImpl(new Impl)
{
  pImpl->argv.push_back(pImpl->nodejsArgv0);
  pImpl->argc = pImpl->argv.size();

  spdlog::info("JsEngine::JsEngine() - Enter");
  spdlog::info("JsEngine::JsEngine() - Initializing NodeInstance");

  pImpl->nodeInstance = std::make_unique<NodeInstance>();
  int initResult = pImpl->nodeInstance->Init(pImpl->argc, pImpl->argv.data());

  if (initResult != 0) {
    spdlog::error(
      "JsEngine::JsEngine() - Failed to initialize NodeInstance: {}",
      GetError());
    pImpl->nodeInstance.reset();
    return;
  }

  spdlog::info("JsEngine::JsEngine() - NodeInstance initialized");

  spdlog::info("JsEngine::JsEngine() - Creating environment");

  int createEnvironmentResult = pImpl->nodeInstance->CreateEnvironment(
    pImpl->argc, pImpl->argv.data(), &pImpl->env);

  if (createEnvironmentResult != 0) {
    spdlog::error("JsEngine::JsEngine() - Failed to create environment: {}",
                  GetError());
    pImpl->nodeInstance.reset();
    return;
  }

  spdlog::info("JsEngine::JsEngine() - Environment created");

  spdlog::info(
    "JsEngine::JsEngine() - Copmpiling script V8SCRIPT_INIT_SKYRIM_PLATFORM");

  pImpl->nodeInstance->ClearJavaScriptError();

  int compileResult = pImpl->nodeInstance->CompileScript(
    pImpl->env, kInitSkyrimPlatformScript, V8SCRIPT_INIT_SKYRIM_PLATFORM);
  if (compileResult != 0) {
    spdlog::error("JsEngine::JsEngine() - Failed to compile script: {}",
                  GetError());
    pImpl->nodeInstance.reset();
    return;
  }

  spdlog::info("JsEngine::JsEngine() - Copmpiling script "
               "V8SCRIPT_CALL_PREPARED_FUNCTION");

  pImpl->nodeInstance->ClearJavaScriptError();

  compileResult = pImpl->nodeInstance->CompileScript(
    pImpl->env, kCallPreparedFunctionSсript, V8SCRIPT_CALL_PREPARED_FUNCTION);
  if (compileResult != 0) {
    spdlog::error("JsEngine::JsEngine() - Failed to compile script: {}",
                  GetError());
    pImpl->nodeInstance.reset();
    return;
  }

  spdlog::info(
    "JsEngine::JsEngine() - Executing script V8SCRIPT_INIT_SKYRIM_PLATFORM");

  pImpl->nodeInstance->ClearJavaScriptError();

  int executeResult = pImpl->nodeInstance->ExecuteScript(
    pImpl->env, V8SCRIPT_INIT_SKYRIM_PLATFORM);
  if (executeResult != 0) {
    spdlog::error("JsEngine::JsEngine() - Failed to execute script: {}",
                  GetError());
    pImpl->nodeInstance.reset();
    return;
  }

  std::string javaScriptError = pImpl->nodeInstance->GetJavaScriptError();
  if (!javaScriptError.empty()) {
    spdlog::info("JsEngine::JsEngine() - JavaScript error: {}",
                 javaScriptError);
  }
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
  JsEngine::GetSingleton()->pImpl->retrievedEnv = info.Env();

  auto f = JsEngine::GetSingleton()->pImpl->preparedFunctionsStack.top();

  if (f) {
    f(info.Env());
  } else {
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
  spdlog::info("InitNativeAddon() - env {:x}",
               reinterpret_cast<uint64_t>(static_cast<napi_env>(env)));
  exports.Set("callPreparedFunction",
              Napi::Function::New(
                env, NapiHelper::WrapCppExceptions(CallPreparedFunction)));
  return exports;
}

NODE_API_MODULE(skyrim_platform, InitNativeAddon)
