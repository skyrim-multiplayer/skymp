#pragma once
#include "JsExternalObjectBase.h"
#include "JsFunctionArguments.h"
#include "JsType.h"
#include "FunctionT.h"
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

class JsValue
{
  friend class JsEngine;
  friend class ChakraBackendUtils;
public:
  static JsValue Undefined();
  static JsValue Null();
  static JsValue Object();
  static JsValue ExternalObject(JsExternalObjectBase* data);
  static JsValue Array(uint32_t n);
  static JsValue GlobalObject();
  static JsValue Bool(bool arg);
  static JsValue String(const std::string& arg);
  static JsValue Int(int arg);
  static JsValue Double(double arg);
  static JsValue Function(const FunctionT& arg);
  static JsValue NamedFunction(const char* name, const FunctionT& arg);
  static JsValue Uint8Array(uint32_t length);
  static JsValue ArrayBuffer(uint32_t length);
  void* GetTypedArrayData() const;
  uint32_t GetTypedArrayBufferLength() const;
  void* GetArrayBufferData() const;
  uint32_t GetArrayBufferLength() const;

  JsValue();
  JsValue(const std::string& arg);
  JsValue(const char* arg);
  JsValue(int arg);
  JsValue(double arg);
  JsValue(const std::vector<JsValue>& arg);
  JsValue(const JsValue& arg);
  JsValue& operator=(const JsValue& arg);
  ~JsValue();

  std::string ToString() const;

  operator bool() const;
  operator std::string() const;
  operator int() const;
  operator double() const;

  JsType GetType() const;

  JsExternalObjectBase* GetExternalData() const;

  // SetProperty is const because this doesn't modify JsValue itself
  void SetProperty(const JsValue& key, const JsValue& newValue) const;

  // SetProperty is const because this doesn't modify JsValue itself
  void SetProperty(const char* propertyName, const FunctionT& getter,
                   const FunctionT& setter) const;

  JsValue GetProperty(const JsValue& key) const;

  JsValue Call(const std::vector<JsValue>& arguments) const;
  JsValue Constructor(const std::vector<JsValue>& arguments) const;

private:
  explicit JsValue(void* internalJsRef);
  void AddRef();
  void Release();

  JsValue Call(const std::vector<JsValue>& arguments, bool isConstructor) const;

#ifdef JS_ENGINE_TRACING_ENABLED
  void TraceConstructor();

  void TraceDestructor();

  static std::map<void*, std::string>& GetStringValuesStorage();

  static std::map<void*, std::vector<uint32_t>>& GetJsValueIdStorage();

  static uint32_t& GetJsValueNextId();
#else
  void TraceConstructor() {}
  void TraceDestructor() {}
#endif

  void* value = nullptr;
};