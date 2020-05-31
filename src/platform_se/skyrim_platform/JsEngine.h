#pragma once
#include "TaskQueue.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class JsFunctionArguments;
class JsPropertyKey;

class JsExternalObjectBase
{
public:
  virtual ~JsExternalObjectBase() = default;
};

class JsValue
{
public:
  friend class JsValueAccess;

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

  static JsValue Undefined();
  static JsValue Null();
  static JsValue Object();
  static JsValue ExternalObject(JsExternalObjectBase* data);
  static JsValue Array(uint32_t n);
  static JsValue GlobalObject();
  static JsValue Function(const FunctionT&);
  static JsValue NamedFunction(const char* name, const FunctionT&);
  static JsValue Bool(bool);
  static JsValue String(const std::string&);
  static JsValue Int(int);
  static JsValue Double(double);

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

  JsValue(const JsValue&);
  JsValue& operator=(const JsValue&);

  ~JsValue();

  std::string ToString() const;

  explicit operator bool() const;
  explicit operator std::string() const;
  explicit operator int() const;
  explicit operator double() const;

  Type GetType() const;
  JsExternalObjectBase* GetExternalData() const;

  // SetProperty is const because this doesn't modify JsValue itself
  void SetProperty(const JsValue& key, const JsValue& value) const;
  void SetProperty(const char* propertyName, const FunctionT& getter,
                   const FunctionT& setter) const;

  JsValue GetProperty(const JsValue& key) const;

  JsValue Call(const std::vector<JsValue>& arguments) const
  {
    return Call(arguments, false);
  }

  JsValue Constructor(const std::vector<JsValue>& arguments) const
  {
    return Call(arguments, true);
  }

private:
  static void* NativeFunctionImpl(void* callee, bool isConstructorCall,
                                  void** arguments,
                                  unsigned short argumentsCount,
                                  void* callbackState);

  JsValue Call(const std::vector<JsValue>& arguments,
               bool isConstructorCall) const;

  explicit JsValue(void* internalJsRef);
  void AddRef();
  void Release();

  void* value = nullptr;
};

class JsFunctionArguments
{
public:
  virtual size_t GetSize() const noexcept = 0;
  virtual const JsValue& operator[](size_t i) const noexcept = 0;
};

class JsEngine
{
public:
  JsEngine();
  ~JsEngine();

  JsValue RunScript(const std::string& source, const std::string& fileName);

  void ResetContext(TaskQueue* taskQueue);

  size_t GetMemoryUsage() const;

private:
  struct Impl;

  Impl* const pImpl;
};