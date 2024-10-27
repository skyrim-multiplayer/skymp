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
  throw 1;
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
    size_t errorLength = pImpl->nodeInstance->GetError(nullptr, 0);
    std::vector<char> errorBuffer(errorLength);
    pImpl->nodeInstance->GetError(errorBuffer.data(), errorLength);
    std::string error(errorBuffer.data(), errorLength);

    spdlog::error("Failed to initialize NodeInstance: {}", error);
    //throw std::runtime_error("Failed to initialize NodeInstance: " + error);
  }

  spdlog::info("JsEngine::JsEngine() - NodeInstance initialized");
}
