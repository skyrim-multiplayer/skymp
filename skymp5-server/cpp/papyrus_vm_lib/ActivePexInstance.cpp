#include "OpcodesImplementation.h"
#include "Utils.h"
#include "VirtualMachine.h"
#include <algorithm>
#include <cassert>
#include <cctype> // tolower
#include <functional>
#include <sstream>
#include <stdexcept>

namespace {
bool IsSelfStr(const VarValue& v)
{
  return v.GetType() == VarValue::Type::String &&
    !Utils::stricmp("self", static_cast<const char*>(v));
}
}

ActivePexInstance::ActivePexInstance()
{
  this->parentVM = nullptr;
}

ActivePexInstance::ActivePexInstance(const PexScript::Lazy& sourcePex,
    const std::shared_ptr<IVariablesHolder>& mapForFillProperties,
    VirtualMachine* parentVM, const VarValue& activeInstanceOwner,
    const std::string& childrenName)
{
  this->childrenName = childrenName;
  this->activeInstanceOwner = activeInstanceOwner;
  this->parentVM = parentVM;
  this->sourcePex = sourcePex;
  this->parentInstance =
    FillParentInstance(sourcePex.fn()->objectTable[0].parentClassName,
                       activeInstanceOwner, mapForFillProperties);

  this->variables = mapForFillProperties;

  this->isValid = true;
}

std::shared_ptr<ActivePexInstance> ActivePexInstance::FillParentInstance(
  const std::string& nameNeedScript, VarValue activeInstanceOwner_,
  const std::shared_ptr<IVariablesHolder>& mapForFillProperties) const
{
  return parentVM->CreateActivePexInstance(nameNeedScript, activeInstanceOwner_,
                                           mapForFillProperties,
                                           this->sourcePex.source);
}

FunctionInfo ActivePexInstance::GetFunctionByName(const char* name_,
                                                  const std::string& stateName_) const
{
  for (const auto& object : sourcePex.fn()->objectTable) {
    for (const auto& [stateName, functions] : object.states) {
      if (stateName != stateName_){
        continue;
      }

      for (const auto& [fName, function] : functions) {
          if (name_ == fName) {
           FunctionInfo result(function);
            result.valid = true;
            return result;
          }
      }
    }
  }
  return FunctionInfo();
}

std::string ActivePexInstance::GetActiveStateName() const
{
  VarValue* var = nullptr;
  try {
    var = variables->GetVariableByName("::State", *sourcePex.fn());
  } catch (...) {
    assert(0 &&
           "GetVariableByName must never throw when '::State' variable is "
           "requested");
  }
  if (!var)
    throw std::runtime_error(
      "'::State' variable doesn't exist in ActivePexInstance");
  return static_cast<const char*>(*var);
}

Object::PropInfo* ActivePexInstance::GetProperty(
  const ActivePexInstance& scriptInstance, const std::string& nameProperty,
  uint8_t flag)
{
  if (!scriptInstance.IsValid())
    return nullptr;

  if (flag == Object::PropInfo::kFlags_Read) {

    for (auto& object : scriptInstance.sourcePex.fn()->objectTable) {
      for (auto& prop : object.properties) {
        if (prop.name == nameProperty &&
            (prop.flags & 5) == prop.kFlags_Read) {
          return &prop;
        }
      }
    }

    if (flag == Object::PropInfo::kFlags_Write) {

      for (auto& object : scriptInstance.sourcePex.fn()->objectTable) {
        for (auto& prop : object.properties) {
          if (prop.name == nameProperty &&
              (prop.flags & 6) == prop.kFlags_Write) {
            return &prop;
          }
        }
      }
    }
  }

  return nullptr;
}

const std::string& ActivePexInstance::GetSourcePexName() const
{
  if (!sourcePex.fn()) {
    static const std::string empty = "";
    return empty;
  }

  return sourcePex.fn()->source;
}

VarValue CastToString(const VarValue& var)
{
  switch (var.GetType()) {
  case VarValue::Type::Object: {
    if (auto * ptr = static_cast<IGameObject*>(var); ptr)
    {
       return VarValue(ptr->GetStringID());
    }

    constexpr static std::string_view noneString = "None";
    return VarValue(noneString.data());
    }
    case VarValue::Type::Identifier:
      assert(false);
      return VarValue();
    case VarValue::Type::String:
      return var;
    case VarValue::Type::Integer:
      return VarValue(std::to_string(static_cast<int32_t>(var)));
    case VarValue::Type::Float: {
      char buffer[512];
      snprintf(buffer, sizeof(buffer), "%.*g", 9000, static_cast<double>(var));
      return VarValue(std::string(buffer));
    }
    case VarValue::Type::Bool: {
      return VarValue(static_cast<bool>(var) ? "True" : "False");
    }
    case VarValue::Type::ObjectArray:
      return GetElementsArrayAtString(var);
    case VarValue::Type::StringArray:
      return GetElementsArrayAtString(var);
    case VarValue::Type::IntArray:
      return GetElementsArrayAtString(var);
    case VarValue::Type::FloatArray:
      return GetElementsArrayAtString(var);
    case VarValue::Type::BoolArray:
      return GetElementsArrayAtString(var);
    default:
      assert(false);
      return VarValue();
  }
}

VarValue GetElementsArrayAtString(const VarValue& array)
{
  const auto type = array.GetType();
  
  std::string returnValue = "[";

  for (size_t i = 0; i < array.pArray->size(); ++i) {
    switch (type) {
      case VarValue::Type::ObjectArray: {
        auto object = (static_cast<IGameObject*>((*array.pArray)[i]));
        returnValue += object ? object->GetStringID() : "None";
        break;
      }

      case VarValue::Type::StringArray:
        returnValue += (const char*)((*array.pArray)[i]);
        break;

      case VarValue::Type::IntArray:
        returnValue += std::to_string((int)((*array.pArray)[i]));
        break;

      case VarValue::Type::FloatArray:
        returnValue += std::to_string((double)((*array.pArray)[i]));
        break;

      case VarValue::Type::BoolArray: {
        VarValue& temp = ((*array.pArray)[i]);
        returnValue += (const char*)(CastToString(temp));
        break;
      }
      default:
        assert(false);
    }

    if (i < array.pArray->size() - 1)
      returnValue += ", ";
    else
      returnValue += "]";
  }

  return VarValue(returnValue);
}

struct ActivePexInstance::ExecutionContext
{
  std::shared_ptr<StackIdHolder> stackIdHolder;
  std::vector<FunctionCode::Instruction> instructions;
  std::shared_ptr<std::vector<Local>> locals;
  bool needReturn = false;
  bool needJump = false;
  int jumpStep = 0;
  VarValue returnValue = VarValue::None();
  size_t line = 0;
};

std::vector<VarValue> GetArgsForCall(uint8_t op,
                                     const std::vector<VarValue*>& opcodeArgs)
{
  std::vector<VarValue> argsForCall;
  if (opcodeArgs.size() > 4) {
    for (size_t i = 4; i < opcodeArgs.size(); ++i) {
      argsForCall.push_back(*opcodeArgs[i]);
    }
  }
  if (op == FunctionCode::kOp_CallParent && opcodeArgs.size() > 3) {
    for (size_t i = 3; i < opcodeArgs.size(); ++i) {
      argsForCall.push_back(*opcodeArgs[i]);
    }
  }
  return argsForCall;
}

bool ActivePexInstance::EnsureCallResultIsSynchronous(
  const VarValue& callResult, ExecutionContext* ctx)
{
  if (!callResult.promise)
    return true;

  Viet::Promise<VarValue> currentFnPr;

  auto ctxCopy = *ctx;
  callResult.promise->Then([this, ctxCopy, currentFnPr](VarValue v) {
    auto ctxCopy_ = ctxCopy;
    ctxCopy_.line++;
    auto res = ExecuteAll(ctxCopy_, v);

    if (res.promise)
      res.promise->Then(currentFnPr);
    else
      currentFnPr.Resolve(res);
  });

  ctx->needReturn = true;
  ctx->returnValue = VarValue(currentFnPr);
  return false;
}

void ActivePexInstance::ExecuteOpCode(ExecutionContext* ctx, uint8_t op,
                                      const std::vector<VarValue*>& args)
{
  auto argsForCall = GetArgsForCall(op, args);

  switch (op) {
    case OpcodesImplementation::Opcodes::op_Nop:
      break;
    case OpcodesImplementation::Opcodes::op_iAdd:
    case OpcodesImplementation::Opcodes::op_fAdd:
      *args[0] = *args[1] + (*args[2]);
      break;
    case OpcodesImplementation::Opcodes::op_iSub:
    case OpcodesImplementation::Opcodes::op_fSub:
      *args[0] = *args[1] - (*args[2]);
      break;
    case OpcodesImplementation::Opcodes::op_iMul:
    case OpcodesImplementation::Opcodes::op_fMul:
      *args[0] = *args[1] * (*args[2]);
      break;
    case OpcodesImplementation::Opcodes::op_iDiv:
    case OpcodesImplementation::Opcodes::op_fDiv:
      *args[0] = *args[1] / (*args[2]);
      break;
    case OpcodesImplementation::Opcodes::op_iMod:
      *args[0] = *args[1] % (*args[2]);
      break;
    case OpcodesImplementation::Opcodes::op_Not:
      *args[0] = !(*args[1]);
      break;
    case OpcodesImplementation::Opcodes::op_iNeg:
      *args[0] = *args[1] * VarValue(-1);
      break;
    case OpcodesImplementation::Opcodes::op_fNeg:
      *args[0] = *args[1] * VarValue(-1.0f);
      break;
    case OpcodesImplementation::Opcodes::op_Assign:
      *args[0] = *args[1];
      break;
    case OpcodesImplementation::Opcodes::op_Cast:
      switch ((*args[0]).GetType()) {
      case VarValue::Type::Object: {
          auto to = args[0];
          auto from = IsSelfStr(*args[1]) ? &activeInstanceOwner : args[1];
          CastObjectToObject(to, from, *ctx->locals);
        } break;
        case VarValue::Type::Integer:
          *args[0] = (*args[1]).CastToInt();
          break;
        case VarValue::Type::Float:
          *args[0] = (*args[1]).CastToFloat();
          break;
        case VarValue::Type::Bool:
          *args[0] = (*args[1]).CastToBool();
          break;
        case VarValue::Type::String:
          *args[0] = CastToString(*args[1]);
          break;
        default:
          // assert(0);
          // Triggered by some array stuff in SkyMP, not sure this is OK
          *args[0] = (*args[1]);
          break;
      }
      break;
    case OpcodesImplementation::op_Cmp_eq:
      *args[0] = VarValue((*args[1]) == (*args[2]));
      break;
    case OpcodesImplementation::Opcodes::op_Cmp_lt:
      *args[0] = VarValue(*args[1] < *args[2]);
      break;
    case OpcodesImplementation::Opcodes::op_Cmp_le:
      *args[0] = VarValue(*args[1] <= *args[2]);
      break;
    case OpcodesImplementation::Opcodes::op_Cmp_gt:
      *args[0] = VarValue(*args[1] > *args[2]);
      break;
    case OpcodesImplementation::Opcodes::op_Cmp_ge:
      *args[0] = VarValue(*args[1] >= *args[2]);
      break;
    case OpcodesImplementation::Opcodes::op_Jmp:
      ctx->jumpStep = static_cast<int>(*args[0]) - 1;
      ctx->needJump = true;
      break;
    case OpcodesImplementation::Opcodes::op_Jmpt:
      if (static_cast<bool>(*args[0])) {
        ctx->jumpStep = static_cast<int>(*args[1]) - 1;
        ctx->needJump = true;
      }
      break;
    case OpcodesImplementation::Opcodes::op_Jmpf:
      if (!static_cast<bool>(*args[0])) {
        ctx->jumpStep = static_cast<int>(*args[1]) - 1;
        ctx->needJump = true;
      }
      break;
    case OpcodesImplementation::Opcodes::op_CallParent: {
      auto parentName =
        parentInstance ? parentInstance->GetSourcePexName() : "";
      try {
        auto gameObject = static_cast<IGameObject*>(activeInstanceOwner);
        auto res = parentVM->CallMethod(gameObject, static_cast<const char*>(*args[0]),
                                        argsForCall);
        if (EnsureCallResultIsSynchronous(res, ctx))
          *args[1] = res;
      } catch (std::exception& e) {
        if (auto handler = parentVM->GetExceptionHandler())
          handler({ e.what(), sourcePex.fn()->source });
        else
          throw;
      }
    } break;
    case OpcodesImplementation::Opcodes::op_CallMethod: {
      VarValue* object = IsSelfStr(*args[1]) ? &activeInstanceOwner : args[1];

      // BYOHRelationshipAdoptionPetDoorTrigger
      if (args[0]->GetType() != VarValue::Type::String &&
          args[0]->GetType() != VarValue::Type::Identifier)
        throw std::runtime_error("Anomally in CallMethod. String expected");

      std::string functionName = static_cast<const char*>(*args[0]);
      static const std::string nameOnBeginState = "onBeginState";
      static const std::string nameOnEndState = "onEndState";
      try {
        if (functionName == nameOnBeginState ||
            functionName == nameOnEndState) {
          parentVM->SendEvent(this, functionName.c_str(), argsForCall);
          break;
        }

        auto nullableGameObject = static_cast<IGameObject*>(*object);
        auto res = parentVM->CallMethod(nullableGameObject, functionName.c_str(),
                                 argsForCall, ctx->stackIdHolder);
          if (EnsureCallResultIsSynchronous(res, ctx))
            *args[2] = res;

      } catch (std::exception& e) {
        if (auto handler = parentVM->GetExceptionHandler())
          handler({ e.what(), sourcePex.fn()->source });
        else
          throw;
      }
    } break;
    case OpcodesImplementation::Opcodes::op_CallStatic: {
      const auto* className = static_cast<const char*>(*args[0]);
      const auto* functionName = static_cast<const char*>(*args[1]);

      try {
        auto res = parentVM->CallStatic(className, functionName, argsForCall,
                                        ctx->stackIdHolder);
        if (EnsureCallResultIsSynchronous(res, ctx))
          *args[2] = res;
      } catch (std::exception& e) {
        if (auto handler = parentVM->GetExceptionHandler())
          handler({ e.what(), sourcePex.fn()->source });
        else
          throw;
      }
    } break;
    case OpcodesImplementation::Opcodes::op_Return:
      ctx->returnValue = *args[0];
      ctx->needReturn = true;
      break;
    case OpcodesImplementation::Opcodes::op_StrCat:
      *args[0] = OpcodesImplementation::StrCat(
        *args[1], *args[2], this->sourcePex.fn()->stringTable);
      break;
    case OpcodesImplementation::Opcodes::op_PropGet:
      // PropGet/Set seems to work only in very simple cases covered by unit
      // tests
      if (args[1] != nullptr) {
        std::string nameProperty = static_cast<const char*>(*args[0]);
        auto object = static_cast<IGameObject*>(
          IsSelfStr(*args[1]) ? activeInstanceOwner : *args[1]);

        if (!object)
          object = static_cast<IGameObject*>(activeInstanceOwner);

        if (object && !object->activePexInstances.empty()) {
          const auto& inst = object->activePexInstances.back();

          Object::PropInfo* runProperty =
            GetProperty(*inst, nameProperty, Object::PropInfo::kFlags_Read);

          if (runProperty != nullptr) {
            *args[2] = inst->StartFunction(runProperty->readHandler,
                                           argsForCall, ctx->stackIdHolder);
          }
        }
      } else
        assert(false);
      break;
    case OpcodesImplementation::Opcodes::op_PropSet:
      if (args[1] != nullptr) {
        argsForCall.push_back(*args[2]);
        std::string nameProperty = static_cast<const char*>(*args[0]);
        auto object = static_cast<IGameObject*>(
          IsSelfStr(*args[1]) ? activeInstanceOwner : *args[1]);
        if (!object)
          object = static_cast<IGameObject*>(activeInstanceOwner);
        if (object && !object->activePexInstances.empty()) {
          auto inst = object->activePexInstances.back();
          Object::PropInfo* runProperty =
            GetProperty(*inst, nameProperty, Object::PropInfo::kFlags_Write);
          if (runProperty != nullptr) {
            inst->StartFunction(runProperty->writeHandler, argsForCall,
                                ctx->stackIdHolder);
          }
        }
      } else
        assert(false);
      break;
    case OpcodesImplementation::Opcodes::op_Array_Create:
      (*args[0]).pArray = std::make_shared<std::vector<VarValue>>();
      if (static_cast<int32_t>(*args[1]) > 0) {
        (*args[0]).pArray->resize(static_cast<int32_t>(*args[1]));
        auto type = GetArrayElementType((*args[0]).GetType());
        for (auto& element : *(*args[0]).pArray) {
          element = VarValue(type);
        }
      } else
        assert(0);
      break;
    case OpcodesImplementation::Opcodes::op_Array_Length:
      if ((*args[1]).pArray != nullptr) {
        if ((*args[0]).GetType() == VarValue::Type::Integer) {
          *args[0] = VarValue(static_cast<int32_t>(args[1]->pArray->size()));
        } else if ((*args[0]).GetType() == VarValue::Type::Float) {
          *args[0] = VarValue(static_cast<double>(args[1]->pArray->size()));
        }
      } else
        *args[0] = VarValue(0);
      break;
    case OpcodesImplementation::Opcodes::op_Array_GetElement:
      if ((*args[1]).pArray != nullptr) {
        *args[0] = (*args[1]).pArray->at(static_cast<int32_t>(*args[2]));
      } else {
        *args[0] = VarValue::None();
      }
      break;
    case OpcodesImplementation::Opcodes::op_Array_SetElement:
      if ((*args[0]).pArray != nullptr) {
        (*args[0]).pArray->at(static_cast<int32_t>(*args[1])) = *args[2];
      } else
        assert(0);
      break;
    case OpcodesImplementation::Opcodes::op_Array_FindElement:
      OpcodesImplementation::ArrayFindElement(*args[0], *args[1], *args[2],
                                              *args[3]);
      break;
    case OpcodesImplementation::Opcodes::op_Array_RfindElement:
      OpcodesImplementation::ArrayRFindElement(*args[0], *args[1], *args[2],
                                               *args[3]);
      break;
    default:
      assert(0);
  }
}

std::shared_ptr<std::vector<ActivePexInstance::Local>>
ActivePexInstance::MakeLocals(FunctionInfo& function,
                              std::vector<VarValue>& arguments)
{
  auto locals = std::make_shared<std::vector<Local>>();

  // Fill with function locals
  for (auto& var : function.locals) {
    VarValue temp = VarValue(GetTypeByName(var.type));
    temp.objectType = var.type;
    locals->push_back({ var.name, temp });
  }

  // Fill with function args
  for (size_t i = 0; i < arguments.size(); ++i) {
    VarValue temp = arguments[i];
    temp.objectType = function.params[i].type;

    locals->push_back({ function.params[i].name, temp });
    assert(locals->back().second.GetType() == arguments[i].GetType());
  }

  // ?
  for (size_t i = arguments.size(); i < function.params.size(); ++i) {
    const auto& var_ = function.params[i];

    VarValue temp = VarValue(GetTypeByName(var_.type));
    temp.objectType = var_.type;

    locals->push_back({ var_.name, temp });
  }

  // Dereference identifiers
  for (auto& var : *locals) {
    var.second = GetIdentifierValue(*locals, var.second);
  }

  return locals;
}

// Basically, makes vector<VarValue *> from vector<VarValue>
std::vector<std::pair<uint8_t, std::vector<VarValue*>>>
ActivePexInstance::TransformInstructions(
  std::vector<FunctionCode::Instruction>& instructions,
  std::shared_ptr<std::vector<Local>> locals)
{
  std::vector<std::pair<uint8_t, std::vector<VarValue*>>> opCode;
  for (size_t i = 0; i < instructions.size(); ++i) {

    std::pair<uint8_t, std::vector<VarValue*>> temp;
    temp.first = instructions[i].op;

    for (size_t j = 0; j < instructions[i].args.size(); ++j) {
      temp.second.push_back(&instructions[i].args[j]);
    }
    opCode.push_back(temp);
  }

  // Dereference identifiers
  for (auto& [opcodeId, opcodeArgs] : opCode) {
    size_t dereferenceStart;
    switch (opcodeId) {
      case OpcodesImplementation::Opcodes::op_CallMethod:
        // Do not dereference functionName
        dereferenceStart = 1;
        break;
      case OpcodesImplementation::Opcodes::op_CallStatic:
        // Do not dereference className and functionName
        dereferenceStart = 2;
        break;
      case OpcodesImplementation::Opcodes::op_CallParent:
        // TODO?
        dereferenceStart = 0;
        break;
      default:
        dereferenceStart = 0;
        break;
    }
    for (size_t i = dereferenceStart; i < opcodeArgs.size(); ++i) {
      auto& arg = opcodeArgs[i];
      arg = &GetIdentifierValue(*locals, *arg);
    }
  }

  return opCode;
}

VarValue ActivePexInstance::ExecuteAll(
  ExecutionContext& ctx, std::optional<VarValue> previousCallResult)
{
  auto pipex = sourcePex.fn();

  auto opCode = TransformInstructions(ctx.instructions, ctx.locals);

  if (previousCallResult) {
    const size_t i = ctx.line - static_cast<size_t>(1);
    const size_t resultIdx =
      opCode[i].first == OpcodesImplementation::Opcodes::op_CallParent ? 1 : 2;

    *opCode[i].second[resultIdx] = *previousCallResult;
  }

  assert(opCode.size() == ctx.instructions.size());

  for (; ctx.line < opCode.size(); ++ctx.line) {
    auto& [op, args] = opCode[ctx.line];
    ExecuteOpCode(&ctx, op, args);

    if (ctx.needReturn) {
      ctx.needReturn = false;
      return ctx.returnValue;
    }

    if (ctx.needJump) {
      ctx.needJump = false;
      ctx.line += ctx.jumpStep;
    }
  }
  return ctx.returnValue;
}

VarValue ActivePexInstance::StartFunction(
  FunctionInfo& function, std::vector<VarValue>& arguments,
  std::shared_ptr<StackIdHolder> stackIdHolder)
{
  if (!stackIdHolder)
    throw std::runtime_error("An empty stackIdHolder passed to StartFunction");
  auto locals = MakeLocals(function, arguments);
  ExecutionContext ctx{ stackIdHolder, function.code.instructions, locals };
  return ExecuteAll(ctx);
}

VarValue& ActivePexInstance::GetIdentifierValue(
  std::vector<Local>& locals, VarValue& value, bool treatStringsAsIdentifiers)
{
  if (const auto valueAsString = static_cast<const char*>(value)) {
    if (treatStringsAsIdentifiers &&
        value.GetType() == VarValue::Type::String) {
      return GetVariableValueByName(&locals, valueAsString);
    }
    if (value.GetType() == VarValue::Type::Identifier) {
      return GetVariableValueByName(&locals, valueAsString);
    }
  }
  return value;
}

VarValue::Type ActivePexInstance::GetTypeByName(std::string typeRef)
{

  std::transform(typeRef.begin(), typeRef.end(), typeRef.begin(), tolower);

  if (typeRef == "int") {
    return VarValue::Type::Integer;
  }
  if (typeRef == "float") {
    return VarValue::Type::Float;
  }
  if (typeRef == "string") {
    return VarValue::Type::String;
  }
  if (typeRef == "bool") {
    return VarValue::Type::Bool;
  }
  if (typeRef == "identifier") {
    assert(0);
  }
  if (typeRef == "string[]") {
    return VarValue::Type::StringArray;
  }
  if (typeRef == "int[]") {
    return VarValue::Type::IntArray;
  }
  if (typeRef == "float[]") {
    return VarValue::Type::FloatArray;
  }
  if (typeRef == "bool[]") {
    return VarValue::Type::BoolArray;
  }
  if (typeRef.find("[]") != std::string::npos) {
    return VarValue::Type::ObjectArray;
  }
  if (typeRef == "none") {
    // assert(false);
    return VarValue::Type::Object;
  }
  return VarValue::Type::Object;
}

VarValue::Type ActivePexInstance::GetArrayElementType(const VarValue::Type type)
{
  VarValue::Type returnType;

  switch (type) {
    case VarValue::Type::ObjectArray:
      returnType = VarValue::Type::Object;

      break;
    case VarValue::Type::StringArray:
      returnType = VarValue::Type::String;

      break;
    case VarValue::Type::IntArray:
      returnType = VarValue::Type::Integer;

      break;
    case VarValue::Type::FloatArray:
      returnType = VarValue::Type::Float;

      break;
    case VarValue::Type::BoolArray:
      returnType = VarValue::Type::Bool;

      break;
    default:
      assert(false);
      return VarValue::Type::Object;
  }

  return returnType;
}

VarValue::Type ActivePexInstance::GetArrayTypeByElementType(const VarValue::Type type)
{
  VarValue::Type returnType;

  switch (type) {
    case VarValue::Type::Object:
      returnType = VarValue::Type::ObjectArray;

      break;
    case VarValue::Type::String:
      returnType = VarValue::Type::StringArray;

      break;
    case VarValue::Type::Integer:
      returnType = VarValue::Type::IntArray;

      break;
    case VarValue::Type::Float:
      returnType = VarValue::Type::FloatArray;

      break;
    case VarValue::Type::Bool:
      returnType = VarValue::Type::BoolArray;

      break;
    default:
      assert(false);
      return VarValue::Type::ObjectArray;
  }

  return returnType;
}

void ActivePexInstance::CastObjectToObject(VarValue* result,
                                           VarValue* scriptToCastOwner,
                                           std::vector<Local>& locals)
{
  std::string objectToCastTypeName = scriptToCastOwner->objectType;
  const std::string& resultTypeName = result->objectType;

  if (scriptToCastOwner->GetType() != VarValue::Type::Object ||
      *scriptToCastOwner == VarValue::None()) {
    *result = VarValue::None();
    return;
  }

  auto object = static_cast<IGameObject*>(*scriptToCastOwner);
  if (object) {
    std::string scriptName = object->GetParentNativeScript();
    while (1) {
      if (scriptName.empty()) {
        break;
      }

      if (!Utils::stricmp(resultTypeName.data(), scriptName.data())) {
        *result = *scriptToCastOwner;
        return;
      }

      // TODO: Test this with attention
      // Here is the case when i.e. variable with type 'Form' casts to
      // 'ObjectReference' while it's actually an Actor

      auto myScriptPex = parentVM->GetPexByName(scriptName);

      if (!myScriptPex.fn) {
        break;
      }

      scriptName = myScriptPex.fn()->objectTable[0].parentClassName;
    }
  }

  *result = VarValue::None();
}

bool ActivePexInstance::HasParent(ActivePexInstance* script,
                                  const std::string& castToTypeName)
{
    if (script == nullptr) {
       return false;
    }

    if (script->sourcePex.fn()->source == castToTypeName)
      return true;

    if (script->parentInstance != nullptr && !script->parentInstance->sourcePex.fn()->source.empty()) {
      if (script->parentInstance->sourcePex.fn()->source == castToTypeName) {
        return true;
      }

      return HasParent(script->parentInstance.get(), castToTypeName);
    }

    return false;
}

bool ActivePexInstance::HasChild(ActivePexInstance* script,
                                 const std::string& castToTypeName)
{

  if (script == nullptr) {
       return false;
  }

  if (script->sourcePex.fn()->source == castToTypeName)
      return true;

  return !script->childrenName.empty() && script->childrenName == castToTypeName;
}

VarValue& ActivePexInstance::GetVariableValueByName(std::vector<Local>* locals,
                                                    const std::string& name)
{

  if (name == "self") {
    return activeInstanceOwner;
  }

  if (locals)
    for (auto& var : *locals) {
      if (var.first == name) {
        return var.second;
      }
    }

  try {
    if (variables)
      if (const auto var = variables->GetVariableByName(name.data(), *sourcePex.fn()))
        return *var;
  } catch (std::exception& e) {

    if (const auto handler = parentVM->GetExceptionHandler()) {
      noneVar = VarValue::None();
      handler({ e.what(), sourcePex.fn()->source });
      return noneVar;
    } else
      throw;
  }

  for (auto& _name : identifiersValueNameCache) {
    if (static_cast<const char*>(*_name) == name) {
      return *_name;
    }
  }

  if (parentVM->IsNativeFunctionByNameExisted(GetSourcePexName())) {

    identifiersValueNameCache.emplace_back(std::make_shared<VarValue>(name));
    return *identifiersValueNameCache.back();
  }

  for (const auto& _string : sourcePex.fn()->stringTable.GetStorage()) {
    if (_string == name) {
      identifiersValueNameCache.emplace_back(std::make_shared<VarValue>(name));
      return *identifiersValueNameCache.back();
    }
  }

  for (auto& _string : parentInstance->sourcePex.fn()->stringTable.GetStorage()) {
    if (_string == name) {

    identifiersValueNameCache.emplace_back(std::make_shared<VarValue>(name));
    return *identifiersValueNameCache.back();
    }
  }

  assert(false);
  static VarValue _;
  return _;
}
