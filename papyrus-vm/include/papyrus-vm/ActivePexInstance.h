#pragma once
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "FunctionCode.h"
#include "FunctionInfo.h"
#include "IVariablesHolder.h"
#include "PexScript.h"
#include "Promise.h"
#include "VarValue.h"

class VirtualMachine;
class StackData;

class ActivePexInstance
{
public:
  using Local = std::pair<std::string, VarValue>;

  ActivePexInstance();
  ActivePexInstance(
    PexScript::Lazy sourcePex,
    const std::shared_ptr<IVariablesHolder>& mapForFillProperties,
    VirtualMachine* parentVM, VarValue activeInstanceOwner,
    std::string childrenName);

  FunctionInfo GetFunctionByName(const char* name,
                                 std::string stateName) const;

  VarValue& GetVariableValueByName(std::vector<Local>* optional,
                                   std::string name);

  VarValue& GetIndentifierValue(std::vector<Local>& locals, VarValue& value,
                                bool treatStringsAsIdentifiers = false);

  VarValue StartFunction(FunctionInfo& function,
                         std::vector<VarValue>& arguments,
                         std::shared_ptr<StackData> stackData);

  static uint8_t GetTypeByName(std::string typeRef);
  std::string GetActiveStateName() const;

  bool IsValid() const { return _IsValid; };

  const std::string& GetSourcePexName() const;

  const std::shared_ptr<ActivePexInstance> GetParentInstance() const
  {
    return parentInstance;
  };

  static uint8_t GetArrayElementType(uint8_t type);
  static uint8_t GetArrayTypeByElementType(uint8_t type);

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

  Object::PropInfo* GetProperty(const ActivePexInstance& scriptInstance,
                                std::string nameProperty, uint8_t flag);

  void CastObjectToObject(VarValue* result, VarValue* objectType,
                          std::vector<Local>& locals);

  bool HasParent(ActivePexInstance* script, std::string castToTypeName);
  bool HasChild(ActivePexInstance* script, std::string castToTypeName);

  std::shared_ptr<ActivePexInstance> FillParentInstance(
    std::string nameNeedScript, VarValue activeInstanceOwner,
    const std::shared_ptr<IVariablesHolder>& mapForFillProperties);

  static VarValue TryCastToBaseClass(
    VirtualMachine& vm, const std::string& resultTypeName,
    VarValue* scriptToCastOwner, std::vector<ActivePexInstance::Local>& locals,
    std::vector<std::string>& outClassesStack);

  static VarValue TryCastMultipleInheritance(
    VirtualMachine& vm, const std::string& resultTypeName,
    VarValue* scriptToCastOwner,
    std::vector<ActivePexInstance::Local>& locals);

  bool _IsValid = false;

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
