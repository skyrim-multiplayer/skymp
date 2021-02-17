#pragma once
#include "Promise.h"
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
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

  using Ptr = std::shared_ptr<IGameObject>;

  // 'Actor', 'ObjectReference' and so on. Used for dynamic casts
  virtual const char* GetParentNativeScript() { return ""; }

  virtual bool EqualsByValue(const IGameObject& obj) const { return false; }

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
  union
  {
    IGameObject* id;
    const char* string;
    int32_t i;
    float f;
    bool b;
  } data;

  std::shared_ptr<IGameObject> owningObject;
  int32_t stackId = -1;

public:
  std::string objectType;

  using Ptr = std::shared_ptr<VarValue>;

  enum Type : uint8_t
  {
    kType_Object = 0, // 0 null?
    kType_Identifier, // 1 identifier
    kType_String,     // 2
    kType_Integer,    // 3
    kType_Float,      // 4
    kType_Bool,       // 5

    _ArraysStart = 11,
    kType_ObjectArray = 11,
    kType_StringArray = 12,
    kType_IntArray = 13,
    kType_FloatArray = 14,
    kType_BoolArray = 15,
    _ArraysEnd = 16,
  };

  uint8_t GetType() const { return static_cast<uint8_t>(this->type); }

  VarValue()
  {
    data.id = nullptr;
    type = Type::kType_Object;
  }

  explicit VarValue(uint8_t type);
  explicit VarValue(IGameObject* object);
  explicit VarValue(int32_t value);
  explicit VarValue(const char* value);
  explicit VarValue(const std::string& value);
  explicit VarValue(float value);
  explicit VarValue(bool value);
  explicit VarValue(Viet::Promise<VarValue> promise);
  explicit VarValue(IGameObject::Ptr object);

  VarValue(uint8_t type, const char* value);

  static VarValue None() { return VarValue(); }

  explicit operator bool() const { return this->CastToBool().data.b; }

  explicit operator IGameObject*() const { return this->data.id; }

  explicit operator int() const { return this->CastToInt().data.i; }

  explicit operator float() const { return this->CastToFloat().data.f; }

  explicit operator const char*() const { return this->data.string; }

  std::shared_ptr<std::vector<VarValue>> pArray;

  std::shared_ptr<Viet::Promise<VarValue>> promise;

  std::shared_ptr<std::string> stringHolder;

  int32_t GetMetaStackId() const;
  void SetMetaStackIdHolder(std::shared_ptr<StackIdHolder> stackIdHolder);
  static VarValue AttachTestStackId(VarValue original = VarValue::None(),
                                    int32_t stackId = 108);

  VarValue operator+(const VarValue& argument2);
  VarValue operator-(const VarValue& argument2);
  VarValue operator*(const VarValue& argument2);
  VarValue operator/(const VarValue& argument2);
  VarValue operator%(const VarValue& argument2);
  VarValue operator!();

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

  void Then(std::function<void(VarValue)> cb);

private:
  Type type;
};

using NativeFunction =
  std::function<VarValue(VarValue self, // will be None for global functions
                         std::vector<VarValue> arguments)>;

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

  typedef std::vector<ParamInfo> ParamTable;
  ParamTable params;
  ParamTable locals;

  FunctionCode code;

  bool IsGlobal() const { return flags & (1 << 0); }

  bool IsNative() const { return flags & (1 << 1); }
};

struct ObjectTable
{

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

      typedef std::vector<StateFunction> FnTable;
      FnTable functions;
    };

    typedef std::vector<VarInfo> VarTable;
    VarTable variables;

    typedef std::vector<PropInfo> PropTable;
    PropTable properties;

    typedef std::vector<StateInfo> StateTable;
    StateTable states;
  };

  typedef std::vector<Object> Storage;
  Storage m_data;
};

struct UserFlagTable
{

  struct UserFlag
  {
    std::string name;
    uint8_t idx = 0;
  };

  typedef std::vector<UserFlag> Storage;
  Storage m_data;
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

    size_t GetNumInstructions() { return lineNumbers.size(); }
  };

  typedef std::vector<DebugFunction> Storage;
  Storage m_data;
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

  using Ptr = std::shared_ptr<PexScript>;

  struct Lazy
  {
    std::string source;
    std::function<std::shared_ptr<PexScript>()> fn;
  };

  ScriptHeader header;
  StringTable stringTable;
  DebugInfo debugInfo;
  UserFlagTable userFlagTable;
  ObjectTable objectTable;

  std::string source;
  std::string user;
  std::string machine;
};

struct ActivePexInstance
{
public:
  using Ptr = std::shared_ptr<ActivePexInstance>;
  using Locals = std::vector<std::pair<std::string, VarValue>>;

  ActivePexInstance();
  ActivePexInstance(
    PexScript::Lazy sourcePex,
    const std::shared_ptr<IVariablesHolder>& mapForFillPropertys,
    VirtualMachine* parentVM, VarValue activeInstanceOwner,
    std::string childrenName);

  FunctionInfo GetFunctionByName(const char* name,
                                 std::string stateName) const;

  VarValue& GetVariableValueByName(Locals* optionalLocals, std::string name);

  VarValue& GetIndentifierValue(Locals& locals, VarValue& value,
                                bool treatStringsAsIdentifiers = false);

  VarValue StartFunction(FunctionInfo& function,
                         std::vector<VarValue>& arguments,
                         std::shared_ptr<StackIdHolder> stackIdHolder);

  static uint8_t GetTypeByName(std::string typeRef);
  std::string GetActiveStateName() const;

  bool IsValid() const { return _IsValid; };

  const std::string& GetSourcePexName() const;

  const ActivePexInstance::Ptr GetParentInstance() const
  {
    return parentInstance;
  };

  static uint8_t GetArrayElementType(uint8_t type);
  static uint8_t GetArrayTypeByElementType(uint8_t type);

private:
  struct ExecutionContext;

  std::vector<std::pair<uint8_t, std::vector<VarValue*>>>
  TransformInstructions(std::vector<FunctionCode::Instruction>& sourceOpCode,
                        std::shared_ptr<Locals> locals);

  std::shared_ptr<Locals> MakeLocals(FunctionInfo& function,
                                     std::vector<VarValue>& arguments);

  VarValue ExecuteAll(
    ExecutionContext& ctx,
    std::optional<VarValue> previousCallResult = std::nullopt);

  void ExecuteOpCode(ExecutionContext* ctx, uint8_t op,
                     const std::vector<VarValue*>& arguments);

  bool EnsureCallResultIsSynchronous(const VarValue& callResult,
                                     ExecutionContext* ctx);

  ObjectTable::Object::PropInfo* GetProperty(
    const ActivePexInstance& scriptInstance, std::string nameProperty,
    uint8_t flag);

  void CastObjectToObject(VarValue* result, VarValue* objectType,
                          Locals& locals);

  bool HasParent(ActivePexInstance* script, std::string castToTypeName);
  bool HasChild(ActivePexInstance* script, std::string castToTypeName);

  ActivePexInstance::Ptr FillParentInstanse(
    std::string nameNeedScript, VarValue activeInstanceOwner,
    const std::shared_ptr<IVariablesHolder>& mapForFillPropertys);

  bool _IsValid = false;

  std::string childrenName;

  PexScript::Lazy sourcePex;
  VirtualMachine* parentVM = nullptr;

  VarValue activeInstanceOwner = VarValue::None();

  ActivePexInstance::Ptr parentInstance;

  std::shared_ptr<IVariablesHolder> variables;
  std::vector<VarValue::Ptr> identifiersValueNameCache;

  uint64_t promiseIdx = 0;
  std::map<uint64_t, std::shared_ptr<Viet::Promise<VarValue>>> promises;

  VarValue noneVar = VarValue::None();
};

VarValue CastToString(const VarValue& var, StringTable& stringTable);
VarValue GetElementsArrayAtString(const VarValue& array, uint8_t type,
                                  StringTable& stringTable);