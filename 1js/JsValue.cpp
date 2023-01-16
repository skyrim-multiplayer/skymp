#include "JsValue.h"

#define USE_CHAKRA // TODO: move to CMakeLists.txt

#if defined(USE_CHAKRA)
#include "private/backends/ChakraBackend.h"
#define BACKEND ChakraBackend
#elif defined(USE_NODE_API)
#include "private/backends/NodeApiBackend.h"
#define BACKEND NodeApiBackend
#else
#error "No backend defined"
#endif

JsValue JsValue::Undefined() {
    return JsValue(BACKEND::Undefined());
}

JsValue JsValue::Null() {
    return JsValue(BACKEND::Null());
}

JsValue JsValue::Object() {
    return JsValue(BACKEND::Object());
}

JsValue JsValue::ExternalObject(JsExternalObjectBase* data) {
    return JsValue(BACKEND::ExternalObject(data));
}

JsValue JsValue::Array(uint32_t n) {
    return JsValue(BACKEND::Array(n));
}

JsValue JsValue::GlobalObject() {
    return JsValue(BACKEND::GlobalObject());
}

JsValue JsValue::Bool(bool arg) {
    return JsValue(BACKEND::Bool(arg));
}

JsValue JsValue::String(const std::string& arg) {
    return JsValue(BACKEND::String(arg));
}

JsValue JsValue::Int(int arg) {
    return JsValue(BACKEND::Int(arg));
}

JsValue JsValue::Double(double arg) {
    return JsValue(BACKEND::Double(arg));
}

JsValue JsValue::Function(const FunctionT& arg) {
    return JsValue(BACKEND::Function(arg));
}

JsValue JsValue::NamedFunction(const char* name, const FunctionT& arg) {
    return JsValue(BACKEND::NamedFunction(name, arg));
}

JsValue JsValue::Uint8Array(uint32_t length) {
    return JsValue(BACKEND::Uint8Array(length));
}

JsValue JsValue::ArrayBuffer(uint32_t length) {
    return JsValue(BACKEND::ArrayBuffer(length));
}

void* JsValue::GetTypedArrayData() const {
    return BACKEND::GetTypedArrayData(value);
}

uint32_t JsValue::GetTypedArrayBufferLength() const {
    return BACKEND::GetTypedArrayBufferLength(value);
}

void* JsValue::GetArrayBufferData() const {
    return BACKEND::GetArrayBufferData(value);
}

uint32_t JsValue::GetArrayBufferLength() const {
    return BACKEND::GetArrayBufferLength(value);
}

JsValue::JsValue()
  {
    *this = Undefined();
    TraceConstructor();
  }

  JsValue::JsValue(const std::string& arg)
  {
    *this = String(arg);
    TraceConstructor();
  }

  JsValue::JsValue(const char* arg)
  {
    *this = String(arg);
    TraceConstructor();
  }

  JsValue::JsValue(int arg)
  {
    *this = Int(arg);
    TraceConstructor();
  }

  JsValue::JsValue(double arg)
  {
    *this = Double(arg);
    TraceConstructor();
  }

  JsValue::JsValue(const std::vector<JsValue>& arg)
  {
    *this = Array(arg.size());
    for (size_t i = 0; i < arg.size(); ++i) {
      SetProperty(Int(i), arg[i]);
    }
    TraceConstructor();
  }

  JsValue::JsValue(const JsValue& arg)
  {
    *this = arg;
    TraceConstructor();
  }

  JsValue& JsValue::operator=(const JsValue& arg)
  {
    if (value) {
      this->Release();
    }
    value = arg.value;
    this->AddRef();
    return *this;
  }

  JsValue::~JsValue()
  {
    TraceDestructor();
    Release();
  }

  std::string JsValue::ToString() const
  {
    void* res = BACKEND::ConvertValueToString(value);
    return BACKEND::GetString(res);
  }

  JsValue::operator bool() const
  {
    return BACKEND::GetBool(value);
  }

  JsValue::operator std::string() const { return BACKEND::GetString(value); }

  JsValue::operator int() const
  {
    return BACKEND::GetInt(value);
  }

  JsValue::operator double() const
  {
    return BACKEND::GetDouble(value);
  }

  JsType JsValue::GetType() const {
    return BACKEND::GetType(value);
  }

  JsExternalObjectBase* JsValue::GetExternalData() const {
    return BACKEND::GetExternalData(value);
  }

  void JsValue::SetProperty(const JsValue& key, const JsValue& newValue) const {
    BACKEND::SetProperty(value, key.value, newValue.value);
  }

  void JsValue::SetProperty(const char* propertyName, const FunctionT& getter,
                   const FunctionT& setter) const {
    JsValue propName = JsValue::String(propertyName);
    BACKEND::DefineProperty(value, propName.value, getter, setter);
  }

  JsValue JsValue::GetProperty(const JsValue& key) const {
    return JsValue(BACKEND::GetProperty(value, key.value));
  }

  JsValue JsValue::Call(const std::vector<JsValue>& arguments) const {
    return Call(arguments, false);
  }

  JsValue JsValue::Constructor(const std::vector<JsValue>& arguments) const {
    return Call(arguments, true);
  }

  JsValue::JsValue(void* internalJsRef) : value(internalJsRef) {
  }

  void JsValue::AddRef() {
    if (value) {
      BACKEND::AddRef(value);
    }
  }

  void JsValue::Release() {
    if (value) {
      BACKEND::Release(value);
    }
  }

  JsValue JsValue::Call(const std::vector<JsValue>& arguments, bool isConstructor) const {
    JsValue *ptr = const_cast<JsValue*>(arguments.data());
    void **args = reinterpret_cast<void**>(ptr);
    return JsValue(BACKEND::Call(value, args, arguments.size(), isConstructor));
  }

#ifdef JS_ENGINE_TRACING_ENABLED
void JsValue::TraceConstructor()
  {
    spdlog::trace("[!] JsValue {}\n", ToString());
    GetStringValuesStorage()[value] = ToString();
    GetJsValueIdStorage()[value].push_back(GetJsValueNextId()++);
  }

  void JsValue::TraceDestructor()
  {
    auto& stringifiedValue = GetStringValuesStorage()[value];
    auto& ids = GetJsValueIdStorage()[value];
    spdlog::trace("[!] ~JsValue {}; ids = {}\n", stringifiedValue,
                  fmt::join(ids, ", "));
  }

  std::map<void*, std::string>& JsValue::GetStringValuesStorage()
  {
    thread_local std::map<void*, std::string> g_stringValues;
    return g_stringValues;
  }

  std::map<void*, std::vector<uint32_t>>& JsValue::GetJsValueIdStorage()
  {
    thread_local std::map<void*, std::vector<uint32_t>> g_ids;
    return g_ids;
  }

  uint32_t& JsValue::GetJsValueNextId()
  {
    thread_local uint32_t g_nextId = 0;
    return g_nextId;
  }
#endif // JS_ENGINE_TRACING_ENABLED

#undef BACKEND
