#pragma once
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>

class VirtualMachine;
struct PexScript;

class IGameObject
{
public:
  virtual ~IGameObject() = default;
  virtual const char* GetStringID() { return "Virtual Implementation"; };
};

enum class FunctionType
{
  Method,         // 'callmethod' opcode
  GlobalFunction, // 'callstatic' opcode
};

struct VarValue
{

private:
  uint8_t type = 0;

  union
  {
    IGameObject* id;
    const char* string;
    int32_t i = 0;
    float f;
    bool b;
  } data;

public:
  std::string objectType;

  enum valueTypes
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

  uint8_t GetType() const { return this->type; }

  VarValue()
  {
    this->type = 0;
    this->data.id = nullptr;
  }

  explicit VarValue(uint8_t type);
  explicit VarValue(IGameObject* object);
  explicit VarValue(int32_t value);
  explicit VarValue(const char* value);
  explicit VarValue(float value);
  explicit VarValue(bool value);

  VarValue::VarValue(uint8_t type, const char* value);

  static VarValue None() { return VarValue(); }

  explicit operator bool() const { return this->CastToBool().data.b; }

  explicit operator IGameObject*() const { return this->data.id; }

  explicit operator int() const { return this->CastToInt().data.i; }

  explicit operator float() const { return this->CastToFloat().data.f; }

  explicit operator const char*() const { return this->data.string; }

  std::shared_ptr<std::vector<VarValue>> pArray;

  VarValue operator+(const VarValue& argument2);
  VarValue operator-(const VarValue& argument2);
  VarValue operator*(const VarValue& argument2);
  VarValue operator/(const VarValue& argument2);
  VarValue operator%(const VarValue& argument2);
  VarValue operator!();

  VarValue& operator=(const VarValue& argument2);


  bool operator==(const VarValue& argument2);
  bool operator>(const VarValue& argument2);
  bool operator>=(const VarValue& argument2);
  bool operator<(const VarValue& argument2);
  bool operator<=(const VarValue& argument2);

  VarValue CastToInt() const;
  VarValue CastToFloat() const;
  VarValue CastToBool() const;
};

using NativeFunction =
  std::function<VarValue(VarValue self, // will be None for global functions
                         std::vector<VarValue> arguments)>;

using VarForBuildActivePex =
  std::map<std::string, std::vector<std::pair<std::string, VarValue>>>;

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

    typedef std::vector<VarValue> VarTable;
    VarTable args;
  };

  typedef std::vector<Instruction> InstructionList;
  InstructionList instructions;
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

  std::string returnType = "";
  std::string docstring = "";
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

struct StringTable
{
  typedef std::vector<std::string> Storage;
  Storage m_data;
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

  std::string childrenName;

  std::shared_ptr<PexScript> sourcePex = nullptr;
  VarValue activeInstanceOwner = VarValue::None();
  VirtualMachine* parentVM = nullptr;

  std::shared_ptr<ActivePexInstance> parentInstance;

  std::vector<ObjectTable::Object::VarInfo> variables;

  std::vector<std::shared_ptr<VarValue>> identifiersValueNameCache;
  std::vector<std::shared_ptr<std::string>> instanceStringTable;

  ActivePexInstance();
  ActivePexInstance(std::shared_ptr<PexScript> sourcePex,
                    VarForBuildActivePex mapForFillPropertys,
                    VirtualMachine* parentVM, VarValue activeInstanceOwner,
                    std::string childrenName);

  FunctionInfo GetFunctionByName(const char* name, std::string stateName);

  VarValue& GetVariableValueByName(
    std::vector<std::pair<std::string, VarValue>>& locals, std::string name);

  VarValue& GetIndentifierValue(
    std::vector<std::pair<std::string, VarValue>>& locals, VarValue& value);

  VarValue CastToString(const VarValue& var);

  VarValue StartFunction(FunctionInfo& function,
                         std::vector<VarValue>& arguments);

  uint8_t GetTypeByName(std::string typeRef);
  std::string GetActiveStateName();

private:
  ObjectTable::Object::PropInfo* GetProperty(ActivePexInstance* scriptInstance,
                                             std::string nameProperty,
                                             uint8_t flag);

  ActivePexInstance* GetActivePexInObject(VarValue* object,
                                          std::string& scriptType);

  std::vector<ObjectTable::Object::VarInfo> FillVariables(
    std::shared_ptr<PexScript> sourcePex,
    std::vector<std::pair<std::string, VarValue>> argsForFillPropertys);

  uint8_t GetArrayElementType(uint8_t type);

  void CastObjectToObject(
    VarValue* result, VarValue* objectType,
    std::vector<std::pair<std::string, VarValue>>& locals);

  bool HasParent(ActivePexInstance* script, std::string castToTypeName);
  bool HasChild(ActivePexInstance* script, std::string castToTypeName);

  std::shared_ptr<ActivePexInstance> FillParentInstanse(
    std::string nameNeedScript, VarValue activeInstanceOwner,
    VarForBuildActivePex mapForFillPropertys);

  VarValue GetElementsArrayAtString(const VarValue& array, uint8_t type);
};
