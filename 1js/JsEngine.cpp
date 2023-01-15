#include "JsEngine.h"
#include "private/backends/ChakraBackend.h"

struct JsEngine::Impl {
    std::vector<std::shared_ptr<std::string>> scriptSrcHolder;
};

JsEngine::JsEngine()
  : pImpl(new Impl)
{
    ChakraBackend::Create();
}

JsEngine::~JsEngine()
{
    ChakraBackend::Destroy();
  delete pImpl;
}

JsValue JsEngine::RunScript(const std::string& src, const std::string& fileName)
{
    // TODO: cleanup scriptSource/scriptSrcHolder properly. currently it grows forever on every script run
    pImpl->scriptSrcHolder.push_back(std::make_shared<std::string>(src));

    void* result = ChakraBackend::RunScript(pImpl->scriptSrcHolder.back()->c_str(), fileName.c_str());

    return result ? JsValue(result) : JsValue::Undefined();
}

void JsEngine::ResetContext(Viet::TaskQueue& taskQueue)
{
    ChakraBackend::ResetContext(taskQueue);
}

size_t JsEngine::GetMemoryUsage() const
{
  return ChakraBackend::GetMemoryUsage();
}