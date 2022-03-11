#pragma once
#include "Promise.h"
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class VirtualMachine;
struct PexScript;
class ActivePexInstance;
class StackIdHolder;

class IGameObject
{
  friend class VirtualMachine;
  friend class ActivePexInstance;

public:
  virtual ~IGameObject() = default;
  virtual const char* GetStringID() { return "Virtual Implementation"; };

  // 'Actor', 'ObjectReference' and so on. Used for dynamic casts
  virtual const char* GetParentNativeScript() { return ""; }

  [[nodiscard]] virtual bool EqualsByValue(const IGameObject& obj) const { return false; }

  bool HasScript(const char* name) const;

private:
  std::vector<std::shared_ptr<ActivePexInstance>> activePexInstances;
};

enum class FunctionType
{
  Method,         // 'callmethod' opcode
  GlobalFunction, // 'callstatic' opcode
};

struct VarValue
{

private:
  union Data
  {
    constexpr Data() : id(nullptr) {}
    constexpr explicit Data(std::nullptr_t) : Data() {}
    constexpr explicit Data(IGameObject* obj) : id(obj) {}
    constexpr explicit Data(const int32_t value) : i(value) {}
    constexpr explicit Data(const double value) : f(value) {}
    constexpr explicit Data(const bool value) : b(value) {}
    constexpr explicit Data(const char* value) : string(value) {}

    IGameObject* id;
    const char* string;
    int32_t i;
    double f;
    bool b;
  };

  Data data;

  std::shared_ptr<IGameObject> owningObject;
  int32_t stackId = -1;

  static constexpr inline double EPSILON = 1e-5;

public:
  std::string objectType;

  enum class Type : uint8_t
  {
    Object = 0, // 0 null?
    Identifier, // 1 identifier
    String,     // 2
    Integer,    // 3
    Float,      // 4
    Bool,       // 5
    ArraysStart = 11,
    ObjectArray = 11,
    StringArray = 12,
    IntArray = 13,
    FloatArray = 14,
    BoolArray = 15,
    ArraysEnd = 16,
    None = 255,
  };

  [[nodiscard]] bool IsArray() const noexcept {return type == Type::ObjectArray ||
                                                      type == Type::StringArray ||
                                                      type == Type::IntArray || 
                                                      type == Type::FloatArray ||
                                                      type == Type::BoolArray;}

  [[nodiscard]] Type GetType() const noexcept { return type; }

  VarValue() : data{nullptr}, type(Type::Object) {}

  explicit VarValue(Type type);
  explicit VarValue(const std::string& value);
  explicit VarValue(int32_t value);
  explicit VarValue(const char* value, Type type_ = Type::String);
  explicit VarValue(double value);
  explicit VarValue(bool value);
  explicit VarValue(IGameObject* object);
  explicit VarValue(const Viet::Promise<VarValue>& promise);
  explicit VarValue(const std::shared_ptr<IGameObject>& object);

  static VarValue None() { return VarValue(); }

  explicit operator bool() const { return this->CastToBool().data.b; }

  explicit operator IGameObject*() const { return this->data.id; }

  explicit operator int() const { return this->CastToInt().data.i; }

  explicit operator double() const { return this->CastToFloat().data.f; }

  explicit operator const char*() const { return this->data.string; }

  std::shared_ptr<std::vector<VarValue>> pArray;

  std::shared_ptr<Viet::Promise<VarValue>> promise;

  std::shared_ptr<std::string> stringHolder;

  int32_t GetMetaStackId() const;
  void SetMetaStackIdHolder(const std::shared_ptr<StackIdHolder>& stackIdHolder);
  static VarValue AttachTestStackId(VarValue original = None(), int32_t stackId = 108);

  VarValue operator+(const VarValue& argument2) const;
  VarValue operator-(const VarValue& argument2) const;
  VarValue operator*(const VarValue& argument2) const;
  VarValue operator/(const VarValue& argument2) const;
  VarValue operator%(const VarValue& argument2) const;
  VarValue operator!() const;

  bool operator==(const VarValue& argument2) const;
  bool operator!=(const VarValue& argument2) const;
  bool operator>(const VarValue& argument2) const;
  bool operator>=(const VarValue& argument2) const;
  bool operator<(const VarValue& argument2) const;
  bool operator<=(const VarValue& argument2) const;

  friend std::ostream& operator<<(std::ostream& os, const VarValue& varValue);

  VarValue& operator=(const VarValue& arg2);

  VarValue CastToInt() const;
  VarValue CastToFloat() const;
  VarValue CastToBool() const;

  void Then(std::function<void(VarValue)> cb) const;

private:
  Type type;
};

using NativeFunction =
  std::function<VarValue(VarValue self, std::vector<VarValue> arguments)>;

class IVariablesHolder
{
public:
  virtual ~IVariablesHolder() = default;

  // Must guarantee that no exception would be thrown for '::State' variable
  virtual VarValue* GetVariableByName(const char* name,
                                      const PexScript& pex) = 0;
};

struct FunctionCode
{
  enum
  {
    kOp_Nop = 0,
    kOp_IAdd,
    kOp_FAdd,
    kOp_ISubtract,
    kOp_FSubtract, // 4
    kOp_IMultiply,
    kOp_FMultiply,
    kOp_IDivide,
    kOp_FDivide, // 8
    kOp_IMod,
    kOp_Not,
    kOp_INegate,
    kOp_FNegate, // C
    kOp_Assign,
    kOp_Cast,
    kOp_CompareEQ,
    kOp_CompareLT, // 10
    kOp_CompareLTE,
    kOp_CompareGT,
    kOp_CompareGTE,
    kOp_Jump, // 14
    kOp_JumpT,
    kOp_JumpF,
    kOp_CallMethod,
    kOp_CallParent, // 18
    kOp_CallStatic,
    kOp_Return,
    kOp_Strcat,
    kOp_PropGet, // 1C
    kOp_PropSet,
    kOp_ArrayCreate,
    kOp_ArrayLength,
    kOp_ArrayGetElement, // 20
    kOp_ArraySetElement,
    kOp_Invalid,
  };

  struct Instruction
  {
    uint8_t op = 0;
    std::vector<VarValue> args;
  };

  std::vector<Instruction> instructions;
};

struct FunctionInfo
{
  bool valid = false;

  enum
  {
    kFlags_Read = 1 << 0,
    kFlags_Write = 1 << 1,
  };

  struct ParamInfo
  {
    std::string name;
    std::string type;
  };

  std::string returnType;
  std::string docstring;
  uint32_t userFlags = 0;
  uint8_t flags = 0;

  std::vector<ParamInfo> params;
  std::vector<ParamInfo> locals;

  FunctionCode code;

  bool IsGlobal() const { return flags & (1 << 0); }

  bool IsNative() const { return flags & (1 << 1); }
};

struct Object
{

  std::string NameIndex;

  std::string parentClassName;
  std::string docstring;
  uint32_t userFlags = 0;
  std::string autoStateName;

  struct VarInfo
  {
    std::string name;
    std::string typeName;
    uint32_t userFlags = 0;
    VarValue value = VarValue();
  };

  struct PropInfo
  {
    enum
    {
      kFlags_Read = 1 << 0,
      kFlags_Write = 1 << 1,
      kFlags_AutoVar = 1 << 2,
    };

    std::string name;
    std::string type;
    std::string docstring;
    uint32_t userFlags = 0;
    uint8_t flags = 0; // 1 and 2 are read/write
    std::string autoVarName;

    FunctionInfo readHandler;
    FunctionInfo writeHandler;
  };

  struct StateInfo
  {

    struct StateFunction
    {
      std::string name;
      FunctionInfo function;
    };

    std::string name;

    std::vector<StateFunction> functions;
  };

  std::vector<VarInfo> variables;

  std::vector<PropInfo> properties;

  std::vector<StateInfo> states;
};

struct UserFlag
{
  std::string name;
  uint8_t idx = 0;
};

struct DebugInfo
{
  uint8_t m_flags = 0;
  uint64_t m_sourceModificationTime = 0;

  struct DebugFunction
  {
    std::string objName;
    std::string stateName;
    std::string fnName;
    uint8_t type = 0; // 0-3 valid

    std::vector<uint16_t> lineNumbers; // one per instruction

    size_t GetNumInstructions() const { return lineNumbers.size(); }
  };

  std::vector<DebugFunction> m_data;
};

class StringTable
{
public:
  // Do NOT mutate storage after pex loading. reallocation would make string
  // VarValues invalid
  void SetStorage(std::vector<std::string> newStorage)
  {
    storage = std::move(newStorage);
  }

  std::vector<std::shared_ptr<std::string>> instanceStringTable;

  const std::vector<std::string>& GetStorage() const { return storage; }

private:
  std::vector<std::string> storage;
};

struct ScriptHeader
{
  enum
  {
    kSignature = 0xFA57C0DE,
    kVerMajor = 0x03,
    kVerMinor = 0x01,
    kGameID = 0x0001,
  };

  uint32_t Signature = 0; // 00	FA57C0DE
  uint8_t VerMajor = 0;   // 04	03
  uint8_t VerMinor = 0;   // 05	01
  uint16_t GameID = 0;    // 06	0001
  uint64_t BuildTime = 0; // 08	uint64_t
};

struct PexScript
{
  // Copying PexScript breaks VarValues with strings
  PexScript() = default;
  PexScript(const PexScript&) = delete;
  PexScript& operator=(const PexScript&) = delete;

  struct Lazy
  {
    std::string source;
    std::function<std::shared_ptr<PexScript>()> fn;
  };

  ScriptHeader header;
  StringTable stringTable;
  DebugInfo debugInfo;
  std::vector<UserFlag> userFlagTable;
  std::vector<Object> objectTable;

  std::string source;
  std::string user;
  std::string machine;
};

class ActivePexInstance
{
public:
  using Local = std::pair<std::string, VarValue>;

  ActivePexInstance();
  ActivePexInstance(const PexScript::Lazy& sourcePex,
    const std::shared_ptr<IVariablesHolder>& mapForFillProperties,
    VirtualMachine* parentVM, const VarValue& activeInstanceOwner,
    const std::string& childrenName);

  FunctionInfo GetFunctionByName(const char* name,
                                 const std::string& stateName) const;

  VarValue& GetVariableValueByName(std::vector<Local>* optional,
                                   const std::string& name);

  VarValue& GetIdentifierValue(std::vector<Local>& locals, VarValue& value,
                                bool treatStringsAsIdentifiers = false);

  VarValue StartFunction(FunctionInfo& function,
                         std::vector<VarValue>& arguments,
                         std::shared_ptr<StackIdHolder> stackIdHolder);

  static VarValue::Type GetTypeByName(std::string typeRef);
  [[nodiscard]] std::string GetActiveStateName() const;

  [[nodiscard]] bool IsValid() const { return isValid; };

  [[nodiscard]] const std::string& GetSourcePexName() const;

  const std::shared_ptr<ActivePexInstance>& GetParentInstance() const
  {
    return parentInstance;
  };

  static VarValue::Type GetArrayElementType(VarValue::Type type);
  static VarValue::Type GetArrayTypeByElementType(VarValue::Type type);

private:
  struct ExecutionContext;

  std::vector<std::pair<uint8_t, std::vector<VarValue*>>>
  TransformInstructions(std::vector<FunctionCode::Instruction>& sourceOpCode,
                        std::shared_ptr<std::vector<Local>> locals);

  std::shared_ptr<std::vector<ActivePexInstance::Local>> MakeLocals(
    FunctionInfo& function, std::vector<VarValue>& arguments);

  VarValue ExecuteAll(
    ExecutionContext& ctx,
    std::optional<VarValue> previousCallResult = std::nullopt);

  void ExecuteOpCode(ExecutionContext* ctx, uint8_t op,
                     const std::vector<VarValue*>& arguments);

  bool EnsureCallResultIsSynchronous(const VarValue& callResult,
                                     ExecutionContext* ctx);

  static Object::PropInfo* GetProperty(const ActivePexInstance& scriptInstance,
                                const std::string& nameProperty, uint8_t flag);

  void CastObjectToObject(VarValue* result, VarValue* objectType,
                          std::vector<Local>& locals);

  static bool HasParent(ActivePexInstance* script, const std::string& castToTypeName);
  static bool HasChild(ActivePexInstance* script, const std::string& castToTypeName);

  std::shared_ptr<ActivePexInstance> FillParentInstance(
    const std::string& nameNeedScript, VarValue activeInstanceOwner,
    const std::shared_ptr<IVariablesHolder>& mapForFillProperties) const;

  bool isValid = false;

  std::string childrenName;

  PexScript::Lazy sourcePex;
  VirtualMachine* parentVM = nullptr;

  VarValue activeInstanceOwner = VarValue::None();

  std::shared_ptr<ActivePexInstance> parentInstance;

  std::shared_ptr<IVariablesHolder> variables;
  std::vector<std::shared_ptr<VarValue>> identifiersValueNameCache;

  uint64_t promiseIdx = 0;
  std::map<uint64_t, std::shared_ptr<Viet::Promise<VarValue>>> promises;

  VarValue noneVar = VarValue::None();
};

VarValue CastToString(const VarValue& var);
VarValue GetElementsArrayAtString(const VarValue& array);
