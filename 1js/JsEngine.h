#pragma once
#include "TaskQueue.h"
#include <ChakraCore.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

// #define JS_ENGINE_TRACING_ENABLED
// ^ uncomment or re-generate project files with -DJS_ENGINE_TRACING_ENABLED to
// enable tracing

// Works with SkyrimPlatform but generates GBs of logs.
// We use spdlog::trace. Make sure that your log level allows you to see
// 'trace'.

// Useful for finding static JsValue variables that fail in destructor due to
// undefined static deinitialization order (Chakra is being deinitialized
// before JsValues are)

// Normal debugging doesn't help since every static variable triggers the same
// assert. It doesn't say anything about which line we constructed a
// problematic variable.

// How to use tracing:
// 0. Ensure that assert fails in Chakra internals after unit tests finish
// 1. Define JS_ENGINE_TRACING_ENABLED
// 2. Build Debug config and launch unit tests
// 3. Wait for assertion failure. The last output you see in console should be
// "~JsValue <value>; ids = 1, 2, ..."
// 4. Remember these numbers
// 5. Set a breakpoint in GetJsValueNextId
// 6. Restart unit tests with a debugger attached. Press "Continue" until
// g_nextId becomes the value you have seen previously
// 7. Now you can see where problematic variable is created in the call stack.
// It's usually static/thread_local variable. Removing this specifier would
// solve the problem. However, you better think about performance too: these
// specifiers were added to initialize constants once.

#ifdef JS_ENGINE_TRACING_ENABLED
#  include <map>
#  include <spdlog/spdlog.h>
#endif

#define JS_ENGINE_F(func) func, #func

class JsEngine;
class JsValue;

// 'this' arg is at index 0
class JsFunctionArguments
{
public:
  virtual size_t GetSize() const noexcept = 0;
  virtual const JsValue& operator[](size_t i) const noexcept = 0;
};

class JsExternalObjectBase
{
public:
  virtual ~JsExternalObjectBase() = default;
};

class JsValue
{
public:
  friend class JsEngine;

  enum class Type
  {
    Undefined = 0,
    Null = 1,
    Number = 2,
    String = 3,
    Boolean = 4,
    Object = 5,
    Function = 6,
    Error = 7,
    Array = 8,
    Symbol = 9,
    ArrayBuffer = 10,
    TypedArray = 11,
    DataView = 12,
  };

  using FunctionT = std::function<JsValue(const JsFunctionArguments& args)>;

  static JsValue Undefined()
  {
    JsValueRef v;
    SafeCall(JS_ENGINE_F(JsGetUndefinedValue), &v);
    return JsValue(v);
  }

  static JsValue Null()
  {
    JsValueRef v;
    SafeCall(JS_ENGINE_F(JsGetNullValue), &v);
    return JsValue(v);
  }

  static JsValue Object()
  {
    JsValueRef v;
    SafeCall(JS_ENGINE_F(JsCreateObject), &v);
    return JsValue(v);
  }

  static JsValue ExternalObject(JsExternalObjectBase* data)
  {
    JsValueRef v;
    SafeCall(
      JS_ENGINE_F(JsCreateExternalObject), data,
      [](void* data_) {
        delete reinterpret_cast<JsExternalObjectBase*>(data_);
      },
      &v);
    return JsValue(v);
  }

  static JsValue Array(uint32_t n)
  {
    JsValueRef v;
    SafeCall(JS_ENGINE_F(JsCreateArray), n, &v);
    return JsValue(v);
  }

  static JsValue GlobalObject()
  {
    JsValueRef v;
    SafeCall(JS_ENGINE_F(JsGetGlobalObject), &v);
    return JsValue(v);
  }

  static JsValue Bool(bool arg)
  {
    JsValueRef v;
    SafeCall(JS_ENGINE_F(JsBoolToBoolean), arg, &v);
    return JsValue(v);
  }

  static JsValue String(const std::string& arg)
  {
    JsValueRef v;
    SafeCall(JS_ENGINE_F(JsCreateString), arg.data(), arg.size(), &v);
    return JsValue(v);
  }

  static JsValue Int(int arg)
  {
    JsValueRef v;
    SafeCall(JS_ENGINE_F(JsIntToNumber), arg, &v);
    return JsValue(v);
  }

  static JsValue Double(double arg)
  {
    JsValueRef v;
    SafeCall(JS_ENGINE_F(JsDoubleToNumber), arg, &v);
    return JsValue(v);
  }

  static JsValue Function(const FunctionT& arg)
  {
    JsValueRef v;
    SafeCall(JS_ENGINE_F(JsCreateFunction), NativeFunctionImpl,
             new FunctionT(arg), &v);
    return JsValue(v);
  }

  static JsValue NamedFunction(const char* name, const FunctionT& arg)
  {
    JsValueRef v;
    auto jsName = JsValue::String(name);
    SafeCall(JS_ENGINE_F(JsCreateNamedFunction), jsName.value,
             NativeFunctionImpl, new FunctionT(arg), &v);
    return JsValue(v);
  }

  static JsValue Uint8Array(uint32_t length)
  {
    JsValueRef v;
    SafeCall(JS_ENGINE_F(JsCreateTypedArray),
             JsTypedArrayType::JsArrayTypeUint8, JS_INVALID_REFERENCE, 0,
             length, &v);
    return JsValue(v);
  }

  static JsValue ArrayBuffer(uint32_t length)
  {
    JsValueRef v;
    SafeCall(JS_ENGINE_F(JsCreateArrayBuffer), length, &v);
    return JsValue(v);
  }

  void* GetTypedArrayData() const
  {
    ChakraBytePtr chakraBytePtr = nullptr;
    unsigned int bufferLength = 0;
    JsTypedArrayType typedArrayType = JsTypedArrayType::JsArrayTypeFloat32;
    int elementSize = 0;
    SafeCall(JS_ENGINE_F(JsGetTypedArrayStorage), value, &chakraBytePtr,
             &bufferLength, &typedArrayType, &elementSize);
    return chakraBytePtr;
  }

  uint32_t GetTypedArrayBufferLength() const
  {
    ChakraBytePtr chakraBytePtr = nullptr;
    unsigned int bufferLength = 0;
    JsTypedArrayType typedArrayType = JsTypedArrayType::JsArrayTypeFloat32;
    int elementSize = 0;
    SafeCall(JS_ENGINE_F(JsGetTypedArrayStorage), value, &chakraBytePtr,
             &bufferLength, &typedArrayType, &elementSize);
    return bufferLength;
  }

  void* GetArrayBufferData() const
  {
    ChakraBytePtr chakraBytePtr = nullptr;
    unsigned int bufferLength = 0;
    SafeCall(JS_ENGINE_F(JsGetArrayBufferStorage), value, &chakraBytePtr,
             &bufferLength);
    return chakraBytePtr;
  }

  uint32_t GetArrayBufferLength() const
  {
    ChakraBytePtr chakraBytePtr = nullptr;
    unsigned int bufferLength = 0;
    SafeCall(JS_ENGINE_F(JsGetArrayBufferStorage), value, &chakraBytePtr,
             &bufferLength);
    return bufferLength;
  }

  JsValue()
  {
    *this = Undefined();
    TraceConstructor();
  }
  JsValue(const std::string& arg)
  {
    *this = String(arg);
    TraceConstructor();
  }
  JsValue(const char* arg)
  {
    *this = String(arg);
    TraceConstructor();
  }
  JsValue(int arg)
  {
    *this = Int(arg);
    TraceConstructor();
  }
  JsValue(double arg)
  {
    *this = Double(arg);
    TraceConstructor();
  }
  JsValue(const std::vector<JsValue>& arg)
  {
    *this = Array(arg.size());
    for (size_t i = 0; i < arg.size(); ++i) {
      SetProperty(Int(i), arg[i]);
    }
    TraceConstructor();
  }

  JsValue(const JsValue& arg)
  {
    *this = arg;
    TraceConstructor();
  }

  JsValue& operator=(const JsValue& arg)
  {
    if (value)
      this->Release();
    value = arg.value;
    this->AddRef();
    return *this;
  }

  ~JsValue()
  {
    TraceDestructor();
    Release();
  }

  std::string ToString() const
  {
    JsValueRef res;
    SafeCall(JS_ENGINE_F(JsConvertValueToString), value, &res);
    return GetString(res);
  }

  operator bool() const
  {
    bool res;
    SafeCall(JS_ENGINE_F(JsBooleanToBool), value, &res);
    return res;
  }

  operator std::string() const { return GetString(value); }

  operator int() const
  {
    int res;
    SafeCall(JS_ENGINE_F(JsNumberToInt), value, &res);
    return res;
  }

  operator double() const
  {
    double res;
    SafeCall(JS_ENGINE_F(JsNumberToDouble), value, &res);
    return res;
  }

  Type GetType() const
  {
    JsValueType type;
    SafeCall(JS_ENGINE_F(JsGetValueType), value, &type);
    return static_cast<JsValue::Type>(type);
  }

  JsExternalObjectBase* GetExternalData() const
  {
    void* externalData;
    bool hasExternslData;

    SafeCall(JS_ENGINE_F(JsHasExternalData), value, &hasExternslData);
    if (!hasExternslData)
      return nullptr;

    SafeCall(JS_ENGINE_F(JsGetExternalData), value, &externalData);
    return reinterpret_cast<JsExternalObjectBase*>(externalData);
  }

  JsValue Call(const std::vector<JsValue>& arguments, bool ctor) const
  {
    JsValueRef res;

    JsValueRef undefined;
    SafeCall(JS_ENGINE_F(JsGetUndefinedValue), &undefined);

    auto n = arguments.size();
    JsValueRef* args = nullptr;

    if (n > 0) {
      args = const_cast<JsValueRef*>(
        reinterpret_cast<const JsValueRef*>(arguments.data()));
    } else {
      args = &undefined;
      ++n;
    }

    SafeCall(ctor ? JsConstructObject : JsCallFunction,
             "JsCallFunction/JsConstructObject", value, args, n, &res);
    return JsValue(res);
  }

  // SetProperty is const because this doesn't modify JsValue itself
  void SetProperty(const JsValue& key, const JsValue& newValue) const
  {
    switch (key.GetType()) {
      case Type::Number: {
        SafeCall(JS_ENGINE_F(JsSetIndexedProperty), value, key.value,
                 newValue.value);
      } break;
      case Type::String: {

        auto str = (std::string)key;

        JsPropertyIdRef propId;
        SafeCall(JS_ENGINE_F(JsCreatePropertyId), str.data(), str.size(),
                 &propId);
        SafeCall(JS_ENGINE_F(JsSetProperty), value, propId, newValue.value,
                 true);
      } break;
      default:
        throw std::runtime_error("SetProperty: Bad key type (" +
                                 std::to_string(int(key.GetType())) + ")");
    }
  }

  // SetProperty is const because this doesn't modify JsValue itself
  void SetProperty(const char* propertyName, const FunctionT& getter,
                   const FunctionT& setter) const
  {
    JsValue descriptor = JsValue::Object();
    JsValue propName = JsValue::String(propertyName);
    if (getter)
      descriptor.SetProperty("get", JsValue::Function(getter));
    if (setter)
      descriptor.SetProperty("set", JsValue::Function(setter));
    bool result;
    SafeCall(JS_ENGINE_F(JsObjectDefineProperty), this->value, propName.value,
             descriptor.value, &result);
  }

  JsValue GetProperty(const JsValue& key) const
  {
    switch (key.GetType()) {
      case Type::Number: {
        JsValueRef res;
        SafeCall(JS_ENGINE_F(JsGetIndexedProperty), value, key.value, &res);
        return JsValue(res);
      } break;
      case Type::String: {
        JsPropertyIdRef propId;
        JsValueRef res;

        // Hot path. Platform-specific functions on Windows are faster than the
        // cross-platform equivalents
#ifndef WIN32
        auto str = static_cast<std::string>(key);
        SafeCall(JS_ENGINE_F(JsCreatePropertyId), str.data(), str.size(),
                 &propId);
#else
        const wchar_t* stringPtr;
        size_t stringSize;
        SafeCall(JS_ENGINE_F(JsStringToPointer), key.value, &stringPtr,
                 &stringSize);
        SafeCall(JS_ENGINE_F(JsGetPropertyIdFromName), stringPtr, &propId);
#endif
        SafeCall(JS_ENGINE_F(JsGetProperty), value, propId, &res);
        return JsValue(res);
      } break;
      case Type::Symbol: {
        JsValueRef res;
        JsPropertyIdRef propId;
        SafeCall(JS_ENGINE_F(JsGetPropertyIdFromSymbol), key.value, &propId);
        SafeCall(JS_ENGINE_F(JsGetProperty), value, propId, &res);
        return JsValue(res);
      }
      default:
        throw std::runtime_error("GetProperty: Bad key type (" +
                                 std::to_string(int(key.GetType())) + ")");
    }
  }

  JsValue Call(const std::vector<JsValue>& arguments) const
  {
    return Call(arguments, false);
  }

  JsValue Constructor(const std::vector<JsValue>& arguments) const
  {
    return Call(arguments, true);
  }

  class JsFunctionArgumentsImpl : public JsFunctionArguments
  {
  public:
    JsFunctionArgumentsImpl(JsValueRef* arr_, size_t n_)
      : arr(arr_)
      , n(n_)
    {
      undefined = std::make_unique<JsValue>(JsValue::Undefined());
    }

    size_t GetSize() const noexcept override { return n; }

    const JsValue& operator[](size_t i) const noexcept override
    {
      // A bit ugly reinterpret_cast, but it's a hot path.
      // We do not want to modify the ref counter for each argument.
      // This is also unit tested, so we would know if it breaks.
      return i < n ? reinterpret_cast<const JsValue&>(arr[i]) : *undefined;
    }

  private:
    JsValueRef* const arr;
    const size_t n;
    std::unique_ptr<JsValue> undefined;
  };
private:
  template <class F, class... A>
  static void SafeCall(F func, const char* funcName, A... args)
  {
    auto result = func(args...);
    if (result != JsErrorCode::JsNoError) {
      throw std::runtime_error(GetJsExceptionMessage(funcName, result));
    }
  }

  static std::string ConvertJsExceptionToString(JsValueRef exception)
  {
    try {
      auto stack = JsValue(exception).GetProperty("stack").ToString();
      if (stack == "undefined") {
        throw 1;
      }
      return stack;
    } catch (...) {
      std::stringstream ss;
      ss << JsValue(exception).ToString() << std::endl;
      ss << "<unable to get stack>";
      return ss.str();
    }
  }

  static std::string GetJsExceptionMessage(const char* opName, JsErrorCode ec)
  {
    std::stringstream ss;
    JsValueRef exception;
    if (JsGetAndClearException(&exception) == JsNoError) {
      ss << ConvertJsExceptionToString(exception);
    } else {
      ss << "'" << opName << "' returned error 0x" << std::hex << int(ec);
    }
    return ss.str();
  }

  static void* NativeFunctionImpl(void* callee, bool isConstructorCall,
                                  void** arguments,
                                  unsigned short argumentsCount,
                                  void* callbackState)
  {
    try {
      JsFunctionArgumentsImpl args(arguments, argumentsCount);

      auto f = reinterpret_cast<FunctionT*>(callbackState);
      return (*f)(args).value;
    } catch (std::exception& e) {
      JsValueRef whatStr, err;
      if (JsCreateString(e.what(), strlen(e.what()), &whatStr) == JsNoError &&
          JsCreateError(whatStr, &err) == JsNoError) {
        JsSetException(err);
      }
      return JS_INVALID_REFERENCE;
    }
  }

  explicit JsValue(JsValueRef internalJsRef)
    : value(internalJsRef)
  {
    AddRef();
    TraceConstructor();
  }

  void AddRef()
  {
    if (value) {
      SafeCall(JS_ENGINE_F(JsAddRef), value, nullptr);
    }
  }

  void Release()
  {
    if (value) {
      JsRelease(value, nullptr);
    }
  }

  static std::string GetString(JsValueRef value)
  {
    size_t outLength;
    SafeCall(JS_ENGINE_F(JsCopyString), value, nullptr, 0, &outLength);

    std::string res;
    res.resize(outLength);
    SafeCall(JS_ENGINE_F(JsCopyString), value, res.data(), outLength,
             &outLength);
    return res;
  }

#ifdef JS_ENGINE_TRACING_ENABLED
  void TraceConstructor()
  {
    spdlog::trace("[!] JsValue {}\n", ToString());
    GetStringValuesStorage()[value] = ToString();
    GetJsValueIdStorage()[value].push_back(GetJsValueNextId()++);
  }

  void TraceDestructor()
  {
    auto& stringifiedValue = GetStringValuesStorage()[value];
    auto& ids = GetJsValueIdStorage()[value];
    spdlog::trace("[!] ~JsValue {}; ids = {}\n", stringifiedValue,
                  fmt::join(ids, ", "));
  }

  static std::map<void*, std::string>& GetStringValuesStorage()
  {
    thread_local std::map<void*, std::string> g_stringValues;
    return g_stringValues;
  }

  static std::map<void*, std::vector<uint32_t>>& GetJsValueIdStorage()
  {
    thread_local std::map<void*, std::vector<uint32_t>> g_ids;
    return g_ids;
  }

  static uint32_t& GetJsValueNextId()
  {
    thread_local uint32_t g_nextId = 0;
    return g_nextId;
  }
#else
  void TraceConstructor() {}
  void TraceDestructor() {}
#endif

  JsValueRef value = nullptr;
};

class JsEngine
{
public:
  JsEngine()
    : pImpl(new Impl)
  {
    JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &pImpl->runtime);
  }

  ~JsEngine()
  {
    JsSetCurrentContext(JS_INVALID_REFERENCE);
    JsDisposeRuntime(pImpl->runtime);
    delete pImpl;
  }

  JsValue RunScript(const std::string& src, const std::string& fileName)
  {

    pImpl->scriptSrcHolder.push_back(std::make_shared<std::string>(src));

    JsValueRef scriptSource;
    JsValue::SafeCall(JS_ENGINE_F(JsCreateExternalArrayBuffer),
                      (void*)pImpl->scriptSrcHolder.back()->data(),
                      (unsigned int)pImpl->scriptSrcHolder.back()->size(),
                      nullptr, nullptr, &scriptSource);

    JsValueRef fname;
    JsValue::SafeCall(JS_ENGINE_F(JsCreateString), fileName.data(),
                      fileName.size(), &fname);

    JsValueRef result = nullptr;
    auto jsRunRes = JsRun(scriptSource, pImpl->currentSourceContext++, fname,
                          JsParseScriptAttributeNone, &result);
    if (jsRunRes != JsNoError) {
      JsValueRef exception = nullptr;
      if (JsGetAndClearException(&exception) == JsNoError) {
        std::string str = JsValue::ConvertJsExceptionToString(exception);
        throw std::runtime_error(str);
      } else {
        throw std::runtime_error("JsRun failed");
      }
    }

    return result ? JsValue(result) : JsValue::Undefined();
  }

  void ResetContext(Viet::TaskQueue& taskQueue)
  {
    JsValue::SafeCall(JS_ENGINE_F(JsCreateContext), pImpl->runtime,
                      &pImpl->context);
    JsValue::SafeCall(JS_ENGINE_F(JsSetCurrentContext), pImpl->context);

    JsValue::SafeCall(JS_ENGINE_F(JsSetPromiseContinuationCallback),
                      OnPromiseContinuation, &taskQueue);

    JsValue::SafeCall(JS_ENGINE_F(JsSetHostPromiseRejectionTracker),
                      OnPromiseRejection, &taskQueue);
  }

  size_t GetMemoryUsage() const
  {
    size_t res;
    JsValue::SafeCall(JS_ENGINE_F(JsGetRuntimeMemoryUsage), pImpl->runtime,
                      &res);
    return res;
  }

private:
  static void OnPromiseContinuation(JsValueRef task, void* state)
  {
    // Equivalent of JsValue::JsValue(JsValueRef *)
    JsValue::SafeCall(JS_ENGINE_F(JsAddRef), task, nullptr);

    auto taskQueue = reinterpret_cast<Viet::TaskQueue*>(state);

    // RAII doesn't work properly here. That's why we do not just use JsValue.
    // TaskQueue can be destroyed AFTER Chakra deinitialization and then try
    // destroying tasks with JsValue instances captured.
    // Also JsRelease (and JsValue dtor) MUST be called in the Chakra thread.

    // Transfer internal ChakraCore value pointer. We did AddRef so Chakra
    // isn't going to invalidate this pointer.
    taskQueue->AddTask([task] {
      // Equivalent of JsValue::Call({ JsValue::Undefined() })
      JsValueRef undefined, res;
      JsValue::SafeCall(JS_ENGINE_F(JsGetUndefinedValue), &undefined);
      JsValue::SafeCall(JS_ENGINE_F(JsCallFunction), task, &undefined, 1,
                        &res);

      // Equivalent of JsValue::~JsValue()
      JsRelease(task, nullptr);
    });
  }

  static void OnPromiseRejection(JsValueRef promise, JsValueRef reason_,
                                 bool handled, void* state)
  {
    if (handled) {
      // This indicates that failure is handled on the JavaScript side.
      // No sense to do anything.
      return;
    }
    auto q = reinterpret_cast<Viet::TaskQueue*>(state);
    std::stringstream ss;
    auto reason = JsValue(reason_);
    auto stack = reason.GetProperty("stack").ToString();
    ss << "Unhandled promise rejection" << std::endl;
    ss << ((stack == "undefined") ? reason.ToString()
                                  : reason.ToString() + "\n" + stack);
    std::string str = ss.str();

    // Would throw from next TaskQueue::Update call
    q->AddTask([str = std::move(str)] { throw std::runtime_error(str); });
  }

  struct Impl
  {
    JsRuntimeHandle runtime;
    JsContextRef context;
    unsigned currentSourceContext = 0;
    std::vector<std::shared_ptr<std::string>> scriptSrcHolder;
  };

  Impl* const pImpl;
};

#undef JS_ENGINE_F
