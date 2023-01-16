#include "JsEngine.h"

#include "private/backends/AnyBackend.h"

struct JsEngine::Impl {
    std::vector<std::shared_ptr<std::string>> scriptSrcHolder;
};

  JsEngine JsEngine::CreateChakra() {
    AnyBackend::GetInstanceForCurrentThread() = AnyBackend::MakeChakraBackend();
    return JsEngine(nullptr);
  }
  JsEngine JsEngine::CreateNodeApi(void* env) {
    AnyBackend::GetInstanceForCurrentThread() = AnyBackend::MakeNodeApiBackend();
    return JsEngine(env);
  }

JsEngine::~JsEngine()
{
    BACKEND Destroy();
}

JsValue JsEngine::RunScript(const std::string& src, const std::string& fileName)
{
    // TODO: cleanup scriptSource/scriptSrcHolder properly. currently it grows forever on every script run
    pImpl->scriptSrcHolder.push_back(std::make_shared<std::string>(src));

    void* result = BACKEND RunScript(pImpl->scriptSrcHolder.back()->c_str(), fileName.c_str());

    return result ? JsValue(result) : JsValue::Undefined();
}

void JsEngine::ResetContext(Viet::TaskQueue& taskQueue)
{
    BACKEND ResetContext(taskQueue);
}

size_t JsEngine::GetMemoryUsage() const
{
  return BACKEND GetMemoryUsage();
}

JsEngine::JsEngine(void *implementationDefinedEnv)
  : pImpl(new Impl)
{
    BACKEND Create(implementationDefinedEnv);
}