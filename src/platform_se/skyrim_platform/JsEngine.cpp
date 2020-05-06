#include "JsEngine.h"

#include "NullPointerException.h"
#include <ChakraCore.h>
#include <sstream>
#include <stdexcept>
#include <string>

class JsValueAccess
{
public:
  static JsValue Ctor(JsValueRef raw) { return JsValue(raw); }
};

namespace {

class JsFunctionArgumentsImpl : public JsFunctionArguments
{
public:
  JsFunctionArgumentsImpl(JsValueRef* arr_, size_t n_)
    : arr(arr_)
    , n(n_)
  {
  }

  size_t GetSize() const noexcept { return n; }

  const JsValue& operator[](size_t i) const noexcept
  {
    thread_local auto g_undefined = JsValue::Undefined();
    return i < n ? reinterpret_cast<const JsValue&>(arr[i]) : g_undefined;
  }

private:
  JsValueRef* const arr;
  const size_t n;
};

class JsException : public std::runtime_error
{
public:
  JsException(const char* opName, JsErrorCode ec)
    : runtime_error(What(opName, ec))
  {
  }

private:
  static std::string What(const char* opName, JsErrorCode ec)
  {
    std::stringstream ss;
    JsValueRef exception;
    if (JsGetAndClearException(&exception) == JsNoError) {
      ss << JsValueAccess::Ctor(exception).ToString();
    } else {
      ss << "'" << opName << "' returned error 0x" << std::hex << int(ec);
    }
    return ss.str();
  }
};

#define F(func) func, #func

template <class F, class... A>
void SafeCall(F func, const char* funcName, A... args)
{
  auto result = func(args...);
  if (result != JsErrorCode::JsNoError) {
    throw JsException(funcName, result);
  }
}

class JsValueRefGuard
{
public:
  JsValueRefGuard(JsValueRef v)
    : value(v)
  {
    SafeCall(F(JsAddRef), value, nullptr);
  }
  ~JsValueRefGuard() { JsRelease(value, nullptr); }

  const JsValueRef value;
};
}

JsValue::JsValue(void* internalJsRef)
  : value(internalJsRef)
{
  AddRef();
}

void JsValue::AddRef()
{
  if (value)
    SafeCall(F(JsAddRef), value, nullptr);
}

void JsValue::Release()
{
  if (value)
    JsRelease(value, nullptr);
}

JsValue JsValue::Undefined()
{
  JsValueRef v;
  SafeCall(F(JsGetUndefinedValue), &v);
  return JsValue(v);
}

JsValue JsValue::Null()
{
  JsValueRef v;
  SafeCall(F(JsGetNullValue), &v);
  return JsValue(v);
}

JsValue JsValue::Object()
{
  JsValueRef v;
  SafeCall(F(JsCreateObject), &v);
  return JsValue(v);
}

JsValue JsValue::ExternalObject(JsExternalObjectBase* data)
{
  JsValueRef v;
  SafeCall(
    F(JsCreateExternalObject), data,
    [](void* data_) { delete reinterpret_cast<JsExternalObjectBase*>(data_); },
    &v);
  return JsValue(v);
}

JsValue JsValue::Array(uint32_t n)
{
  JsValueRef v;
  SafeCall(F(JsCreateArray), n, &v);
  return JsValue(v);
}

JsValue JsValue::GlobalObject()
{
  JsValueRef v;
  SafeCall(F(JsGetGlobalObject), &v);
  return JsValue(v);
}

JsValue JsValue::Bool(bool arg)
{
  JsValueRef v;
  SafeCall(F(JsBoolToBoolean), arg, &v);
  return JsValue(v);
}

JsValue JsValue::String(const std::string& arg)
{
  JsValueRef v;
  SafeCall(F(JsCreateString), arg.data(), arg.size(), &v);
  return JsValue(v);
}

JsValue JsValue::Int(int arg)
{
  JsValueRef v;
  SafeCall(F(JsIntToNumber), arg, &v);
  return JsValue(v);
}

JsValue JsValue::Double(double arg)
{
  JsValueRef v;
  SafeCall(F(JsDoubleToNumber), arg, &v);
  return JsValue(v);
}

void* JsValue::NativeFunctionImpl(void* callee, bool isConstructorCall,
                                  void** arguments,
                                  unsigned short argumentsCount,
                                  void* callbackState)
{
  JsFunctionArgumentsImpl args(arguments, argumentsCount);
  try {
    auto f = reinterpret_cast<FunctionT*>(callbackState);
    return (*f)(args).value;
  } catch (std::exception& e) {
    JsValueRef whatStr, err;
    if (JsCreateString(e.what(), strlen(e.what()), &whatStr) == JsNoError &&
        JsCreateError(whatStr, &err) == JsNoError)
      JsSetException(err);
    return JS_INVALID_REFERENCE;
  }
}

JsValue JsValue::Function(const FunctionT& arg)
{
  JsValueRef v;
  SafeCall(F(JsCreateFunction), NativeFunctionImpl, new FunctionT(arg), &v);
  return JsValue(v);
}

JsValue JsValue::NamedFunction(const char* name, const FunctionT& arg)
{
  JsValueRef v;
  auto jsName = JsValue::String(name);
  SafeCall(F(JsCreateNamedFunction), jsName.value, NativeFunctionImpl,
           new FunctionT(arg), &v);
  return JsValue(v);
}

JsValue::JsValue(const JsValue& arg)
{
  *this = arg;
}

JsValue& JsValue::operator=(const JsValue& arg)
{
  if (value)
    this->Release();
  value = arg.value;
  this->AddRef();
  return *this;
}

JsValue::~JsValue()
{
  Release();
}

std::string JsValue::ToString() const
{
  JsValueRef res;
  SafeCall(F(JsConvertValueToString), value, &res);
  return (std::string)JsValue(res);
}

JsValue::operator bool() const
{
  bool res;
  SafeCall(F(JsBooleanToBool), value, &res);
  return res;
}

JsValue::operator std::string() const
{
  size_t outLength;
  SafeCall(F(JsCopyString), value, nullptr, 0, &outLength);

  std::string res;
  res.resize(outLength);
  SafeCall(F(JsCopyString), value, res.data(), outLength, &outLength);
  return res;
}

JsValue::operator int() const
{
  int res;
  SafeCall(F(JsNumberToInt), value, &res);
  return res;
}

JsValue::operator double() const
{
  double res;
  SafeCall(F(JsNumberToDouble), value, &res);
  return res;
}

JsValue::Type JsValue::GetType() const
{
  JsValueType type;
  SafeCall(F(JsGetValueType), value, &type);
  return static_cast<JsValue::Type>(type);
}

JsExternalObjectBase* JsValue::GetExternalData() const
{
  void* externalData;
  bool hasExternslData;

  SafeCall(F(JsHasExternalData), value, &hasExternslData);
  if (!hasExternslData)
    return nullptr;

  SafeCall(F(JsGetExternalData), value, &externalData);
  return reinterpret_cast<JsExternalObjectBase*>(externalData);
}

JsValue JsValue::Call(const std::vector<JsValue>& arguments, bool ctor) const
{
  JsValueRef res;

  thread_local auto undefined = JsValue::Undefined();

  auto n = arguments.size();
  JsValueRef* args = nullptr;

  if (n > 0) {
    args = const_cast<JsValueRef*>(
      reinterpret_cast<const JsValueRef*>(arguments.data()));
  } else {
    args = reinterpret_cast<JsValueRef*>(&undefined);
    ++n;
  }

  SafeCall(ctor ? JsConstructObject : JsCallFunction,
           "JsCallFunction/JsConstructObject", value, args, n, &res);
  return JsValue(res);
}

void JsValue::SetProperty(const JsValue& key, const JsValue& newValue)
{
  switch (key.GetType()) {
    case Type::Number: {
      SafeCall(F(JsSetIndexedProperty), value, key.value, newValue.value);
    } break;
    case Type::String: {

      auto str = (std::string)key;

      JsPropertyIdRef propId;
      SafeCall(F(JsCreatePropertyId), str.data(), str.size(), &propId);
      SafeCall(F(JsSetProperty), value, propId, newValue.value, true);
    } break;
    default:
      throw std::runtime_error("SetProperty: Bad key type (" +
                               std::to_string(int(key.GetType())) + ")");
  }
}

JsValue JsValue::GetProperty(const JsValue& key) const
{
  switch (key.GetType()) {
    case Type::Number: {
      JsValueRef res;
      SafeCall(F(JsGetIndexedProperty), value, key.value, &res);
      return JsValue(res);
    } break;
    case Type::String: {
      JsPropertyIdRef propId;
      JsValueRef res;

      // Hot path. Platform-specific functions on Windows are faster than the
      // cross-platform equivalents
#ifndef WIN32
      auto str = (std::string)key;
      SafeCall(F(JsCreatePropertyId), str.data(), str.size(), &propId);
#else
      const wchar_t* stringPtr;
      size_t stringSize;
      SafeCall(F(JsStringToPointer), key.value, &stringPtr, &stringSize);
      SafeCall(F(JsGetPropertyIdFromName), stringPtr, &propId);
#endif
      SafeCall(F(JsGetProperty), value, propId, &res);
      return JsValue(res);
    } break;
    case Type::Symbol: {
      JsValueRef res;
      JsPropertyIdRef propId;
      SafeCall(F(JsGetPropertyIdFromSymbol), key.value, &propId);
      SafeCall(F(JsGetProperty), value, propId, &res);
      return JsValue(res);
    }
    default:
      throw std::runtime_error("GetProperty: Bad key type (" +
                               std::to_string(int(key.GetType())) + ")");
  }
}

struct JsEngine::Impl
{
  JsRuntimeHandle runtime;
  JsContextRef context;
  unsigned currentSourceContext = 0;
  std::vector<std::shared_ptr<std::string>> scriptSrcHolder;
};

JsEngine::JsEngine()
  : pImpl(new Impl)
{
  JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &pImpl->runtime);
}

JsEngine::~JsEngine()
{
  JsSetCurrentContext(JS_INVALID_REFERENCE);
  JsDisposeRuntime(pImpl->runtime);
  delete pImpl;
}

JsValue JsEngine::RunScript(const std::string& src,
                            const std::string& fileName)
{

  pImpl->scriptSrcHolder.push_back(std::make_shared<std::string>(src));

  JsValueRef scriptSource;
  SafeCall(F(JsCreateExternalArrayBuffer),
           (void*)pImpl->scriptSrcHolder.back()->data(),
           (unsigned int)pImpl->scriptSrcHolder.back()->size(), nullptr,
           nullptr, &scriptSource);

  JsValueRef fname;
  SafeCall(F(JsCreateString), fileName.data(), fileName.size(), &fname);

  JsValueRef result = nullptr;
  auto jsRunRes = JsRun(scriptSource, pImpl->currentSourceContext++, fname,
                        JsParseScriptAttributeNone, &result);
  if (jsRunRes != JsNoError) {
    JsValueRef exception = nullptr;
    if (JsGetAndClearException(&exception) == JsNoError) {
      std::stringstream ss;
      try {
        auto stack =
          JsValueAccess::Ctor(exception).GetProperty("stack").ToString();
        ss << ((stack == "undefined") ? JsException("JsRun", jsRunRes).what()
                                      : stack);
      } catch (...) {
        ss << JsValueAccess::Ctor(exception).ToString() << std::endl
           << "<unable to get stack>";
      }
      throw std::runtime_error(ss.str());
    } else {
      throw std::runtime_error("JsRun failed");
    }
  }

  return result ? JsValueAccess::Ctor(result) : JsValue::Undefined();
}

void JsEngine::ResetContext(TaskQueue* taskQueue)
{
  if (!taskQueue)
    throw NullPointerException("taskQueue");

  SafeCall(F(JsCreateContext), pImpl->runtime, &pImpl->context);
  SafeCall(F(JsSetCurrentContext), pImpl->context);

  SafeCall(
    F(JsSetPromiseContinuationCallback),
    [](JsValueRef task, void* state) {
      std::shared_ptr<JsValue> taskPtr(new JsValue(JsValueAccess::Ctor(task)));
      auto q = reinterpret_cast<TaskQueue*>(state);
      q->AddTask([taskPtr] { taskPtr->Call({}); });
    },
    taskQueue);

  SafeCall(
    F(JsSetHostPromiseRejectionTracker),
    [](JsValueRef promise, JsValueRef reason_, bool handled, void* state) {
      if (handled)
        return;
      auto q = reinterpret_cast<TaskQueue*>(state);
      std::stringstream ss;
      auto reason = JsValueAccess::Ctor(reason_);
      auto stack = reason.GetProperty("stack").ToString();
      ss << "Unhandled promise rejection" << std::endl;
      ss << ((stack == "undefined") ? reason.ToString() : stack);
      std::string str = ss.str();
      q->AddTask([str] { throw std::runtime_error(str); });
    },
    taskQueue);
}

size_t JsEngine::GetMemoryUsage() const
{
  size_t res;
  SafeCall(F(JsGetRuntimeMemoryUsage), pImpl->runtime, &res);
  return res;
}