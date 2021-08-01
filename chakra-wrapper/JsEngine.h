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

#define JS_ENGINE_F(func) func, #func

class JsValueAccess;
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
  friend class JsValueAccess;
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

  static JsValue String(const std::u16string& arg)
  {
    JsValueRef v;
    SafeCall(JS_ENGINE_F(JsCreateStringUtf16),
             reinterpret_cast<const uint16_t*>(arg.data()), arg.size(), &v);
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

  JsValue() { *this = Undefined(); }
  JsValue(const std::string& arg) { *this = String(arg); }
  JsValue(const char* arg) { *this = String(arg); }
  JsValue(int arg) { *this = Int(arg); }
  JsValue(double arg) { *this = Double(arg); }
  JsValue(const std::vector<JsValue>& arg)
  {
    *this = Array(arg.size());
    for (size_t i = 0; i < arg.size(); ++i)
      SetProperty(Int(i), arg[i]);
  }

  JsValue(const JsValue& arg) { *this = arg; }

  JsValue& operator=(const JsValue& arg)
  {
    if (value)
      this->Release();
    value = arg.value;
    this->AddRef();
    return *this;
  }

  ~JsValue() { Release(); }

  std::string ToString() const
  {
    JsValueRef res;
    SafeCall(JS_ENGINE_F(JsConvertValueToString), value, &res);
    return (std::string)JsValue(res);
  }

  operator bool() const
  {
    bool res;
    SafeCall(JS_ENGINE_F(JsBooleanToBool), value, &res);
    return res;
  }

  operator std::string() const
  {
    size_t outLength;
    SafeCall(JS_ENGINE_F(JsCopyString), value, nullptr, 0, &outLength);

    std::string res;
    res.resize(outLength);
    SafeCall(JS_ENGINE_F(JsCopyString), value, res.data(), outLength,
             &outLength);
    return res;
  }

  operator std::wstring() const
  {
    size_t outLength;
    SafeCall(JS_ENGINE_F(JsCopyStringUtf16), value, 0, 0, nullptr, &outLength);

    std::wstring res;
    res.resize(outLength);
    SafeCall(JS_ENGINE_F(JsCopyStringUtf16), value, 0, outLength, reinterpret_cast<short unsigned int*>(res.data()), &outLength);
    return res;
  }

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

private:
  class JsValueRefGuard
  {
  public:
    JsValueRefGuard(JsValueRef v)
      : value(v)
    {
      SafeCall(JS_ENGINE_F(JsAddRef), value, nullptr);
    }
    ~JsValueRefGuard() { JsRelease(value, nullptr); }

    const JsValueRef value;
  };

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

  static void* NativeFunctionImpl(void* callee, bool isConstructorCall,
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

  explicit JsValue(void* internalJsRef)
    : value(internalJsRef)
  {
    AddRef();
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

  void* value = nullptr;
};

class JsValueAccess
{
public:
  static JsValue Ctor(JsValueRef raw) { return JsValue(raw); }
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

    return result ? JsValueAccess::Ctor(result) : JsValue::Undefined();
  }

  void ResetContext(TaskQueue& taskQueue)
  {
    JsValue::SafeCall(JS_ENGINE_F(JsCreateContext), pImpl->runtime,
                      &pImpl->context);
    JsValue::SafeCall(JS_ENGINE_F(JsSetCurrentContext), pImpl->context);

    JsValue::SafeCall(
      JS_ENGINE_F(JsSetPromiseContinuationCallback),
      [](JsValueRef task, void* state) {
        std::shared_ptr<JsValue> taskPtr(
          new JsValue(JsValueAccess::Ctor(task)));
        auto q = reinterpret_cast<TaskQueue*>(state);
        q->AddTask([taskPtr] { taskPtr->Call({}); });
      },
      &taskQueue);

    JsValue::SafeCall(
      JS_ENGINE_F(JsSetHostPromiseRejectionTracker),
      [](JsValueRef promise, JsValueRef reason_, bool handled, void* state) {
        if (handled)
          return;
        auto q = reinterpret_cast<TaskQueue*>(state);
        std::stringstream ss;
        auto reason = JsValueAccess::Ctor(reason_);
        auto stack = reason.GetProperty("stack").ToString();
        ss << "Unhandled promise rejection" << std::endl;
        ss << ((stack == "undefined") ? reason.ToString()
                                      : reason.ToString() + "\n" + stack);
        std::string str = ss.str();
        q->AddTask([str] { throw std::runtime_error(str); });
      },
      &taskQueue);
  }

  size_t GetMemoryUsage() const
  {
    size_t res;
    JsValue::SafeCall(JS_ENGINE_F(JsGetRuntimeMemoryUsage), pImpl->runtime,
                      &res);
    return res;
  }

private:
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
