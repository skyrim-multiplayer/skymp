#include "papyrus-vm/OpcodesImplementation.h"
#include "papyrus-vm/Utils.h"
#include "papyrus-vm/VirtualMachine.h"
#include <algorithm>
#include <cctype> // tolower
#include <functional>
#include <sstream>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace {
bool IsSelfStr(const VarValue& v)
{
  return v.GetType() == VarValue::kType_String &&
    !Utils::stricmp("self", static_cast<const char*>(v));
}
}

ActivePexInstance::ActivePexInstance()
{
  this->parentVM = nullptr;
}

ActivePexInstance::ActivePexInstance(
  PexScript::Lazy sourcePex,
  const std::shared_ptr<IVariablesHolder>& mapForFillProperties,
  VirtualMachine* parentVM, VarValue activeInstanceOwner,
  std::string childrenName)
{
  this->childrenName = childrenName;
  this->activeInstanceOwner = activeInstanceOwner;
  this->parentVM = parentVM;
  this->sourcePex = sourcePex;
  this->parentInstance =
    FillParentInstance(sourcePex.fn()->objectTable[0].parentClassName,
                       activeInstanceOwner, mapForFillProperties);

  this->variables = mapForFillProperties;

  this->_IsValid = true;
}

std::shared_ptr<ActivePexInstance> ActivePexInstance::FillParentInstance(
  std::string nameNeedScript, VarValue activeInstanceOwner,
  const std::shared_ptr<IVariablesHolder>& mapForFillProperties)
{
  return parentVM->CreateActivePexInstance(nameNeedScript, activeInstanceOwner,
                                           mapForFillProperties,
                                           this->sourcePex.source);
}

FunctionInfo ActivePexInstance::GetFunctionByName(const char* name,
                                                  std::string stateName) const
{

  FunctionInfo function;
  for (auto& object : sourcePex.fn()->objectTable) {
    for (auto& state : object.states) {
      if (state.name == stateName) {
        for (auto& func : state.functions) {
          if (!Utils::stricmp(func.name.data(), name)) {
            function = func.function;
            function.valid = true;
            return function;
          }
        }
      }
    }
  }
  return function;
}

std::string ActivePexInstance::GetActiveStateName() const
{
  VarValue* var = nullptr;
  try {
    var = variables->GetVariableByName("::State", *sourcePex.fn());
  } catch (...) {
    throw std::runtime_error(
      " Papyrus VM: GetVariableByName must never throw when "
      "::State variable is  requested");
  }
  if (!var)
    throw std::runtime_error(
      "Papyrus VM: ::State variable doesn't exist in ActivePexInstance");
  return static_cast<const char*>(*var);
}

Object::PropInfo* ActivePexInstance::GetProperty(
  const ActivePexInstance& scriptInstance, std::string nameProperty,
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
    case VarValue::kType_Object: {
      IGameObject* ptr = ((IGameObject*)var);
      if (ptr) {
        return VarValue(ptr->GetStringID());
      } else {
        const static std::string noneString = "None";
        return VarValue(noneString.c_str());
      }
    }
    case VarValue::kType_Identifier:
      throw std::runtime_error(
        "Papyrus VM: failed to get valid type indentifier, ::CastToString()");
    case VarValue::kType_String:
      return var;
    case VarValue::kType_Integer:
      return VarValue(std::to_string(static_cast<int32_t>(var)));
    case VarValue::kType_Float: {
      char buffer[512];
      snprintf(buffer, sizeof(buffer), "%.*g", 9000, static_cast<double>(var));
      return VarValue(std::string(buffer));
    }
    case VarValue::kType_Bool: {
      return VarValue(static_cast<bool>(var) ? "True" : "False");
    }
    case VarValue::kType_ObjectArray:
      return GetElementsArrayAtString(var, var.kType_ObjectArray);
    case VarValue::kType_StringArray:
      return GetElementsArrayAtString(var, var.kType_StringArray);
    case VarValue::kType_IntArray:
      return GetElementsArrayAtString(var, var.kType_IntArray);
    case VarValue::kType_FloatArray:
      return GetElementsArrayAtString(var, var.kType_FloatArray);
    case VarValue::kType_BoolArray:
      return GetElementsArrayAtString(var, var.kType_BoolArray);
    default:
      throw std::runtime_error(
        "Papyrus VM: Received wrong type, ::CastToString()");
  }
}

VarValue GetElementsArrayAtString(const VarValue& array, uint8_t type)
{
  std::string returnValue = "[";

  for (size_t i = 0; i < array.pArray->size(); ++i) {
    switch (type) {
      case VarValue::kType_ObjectArray: {
        auto object = (static_cast<IGameObject*>((*array.pArray)[i]));
        returnValue += object ? object->GetStringID() : "None";
        break;
      }

      case VarValue::kType_StringArray:
        returnValue += (const char*)((*array.pArray)[i]);
        break;

      case VarValue::kType_IntArray:
        returnValue += std::to_string((int)((*array.pArray)[i]));
        break;

      case VarValue::kType_FloatArray:
        returnValue += std::to_string((double)((*array.pArray)[i]));
        break;

      case VarValue::kType_BoolArray: {
        VarValue& temp = ((*array.pArray)[i]);
        returnValue += (const char*)(CastToString(temp));
        break;
      }
      default:
        throw std::runtime_error(
          " Papyrus VM: None of the type values "
          "​​matched catched exception ::GetElementArrayAtString");
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
        case VarValue::kType_Object: {
          auto to = args[0];
          auto from = IsSelfStr(*args[1]) ? &activeInstanceOwner : args[1];
          CastObjectToObject(to, from, *ctx->locals);
        } break;
        case VarValue::kType_Integer:
          *args[0] = (*args[1]).CastToInt();
          break;
        case VarValue::kType_Float:
          *args[0] = (*args[1]).CastToFloat();
          break;
        case VarValue::kType_Bool:
          *args[0] = (*args[1]).CastToBool();
          break;
        case VarValue::kType_String:
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
      ctx->jumpStep = (int)(*args[0]) - 1;
      ctx->needJump = true;
      break;
    case OpcodesImplementation::Opcodes::op_Jmpt:
      if ((bool)(*args[0])) {
        ctx->jumpStep = (int)(*args[1]) - 1;
        ctx->needJump = true;
      }
      break;
    case OpcodesImplementation::Opcodes::op_Jmpf:
      if ((bool)(!(*args[0]))) {
        ctx->jumpStep = (int)(*args[1]) - 1;
        ctx->needJump = true;
      }
      break;
    case OpcodesImplementation::Opcodes::op_CallParent: {
      auto parentName =
        parentInstance ? parentInstance->GetSourcePexName() : "";
      try {
        auto gameObject = static_cast<IGameObject*>(activeInstanceOwner);
        auto res = parentVM->CallMethod(gameObject, (const char*)(*args[0]),
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
      if (args[0]->GetType() != VarValue::kType_String &&
          args[0]->GetType() != VarValue::kType_Identifier)
        throw std::runtime_error("Anomally in CallMethod. String expected");

      std::string functionName = (const char*)(*args[0]);
      static const std::string nameOnBeginState = "onBeginState";
      static const std::string nameOnEndState = "onEndState";
      try {
        if (functionName == nameOnBeginState ||
            functionName == nameOnEndState) {
          parentVM->SendEvent(this, functionName.c_str(), argsForCall);
          break;
        } else {
          auto nullableGameObject = static_cast<IGameObject*>(*object);
          auto res =
            parentVM->CallMethod(nullableGameObject, functionName.c_str(),
                                 argsForCall, ctx->stackIdHolder);
          if (EnsureCallResultIsSynchronous(res, ctx))
            *args[2] = res;
        }
      } catch (std::exception& e) {
        if (auto handler = parentVM->GetExceptionHandler())
          handler({ e.what(), sourcePex.fn()->source });
        else
          throw;
      }
    } break;
    case OpcodesImplementation::Opcodes::op_CallStatic: {
      const char* className = (const char*)(*args[0]);
      const char* functionName = (const char*)(*args[1]);
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
        std::string nameProperty = (const char*)*args[0];
        auto object = static_cast<IGameObject*>(
          IsSelfStr(*args[1]) ? activeInstanceOwner : *args[1]);
        if (!object)
          object = static_cast<IGameObject*>(activeInstanceOwner);
        if (object && object->activePexInstances.size() > 0) {
          auto inst = object->activePexInstances.back();
          Object::PropInfo* runProperty =
            GetProperty(*inst, nameProperty, Object::PropInfo::kFlags_Read);
          if (runProperty != nullptr) {
            *args[2] = inst->StartFunction(runProperty->readHandler,
                                           argsForCall, ctx->stackIdHolder);
          }
        }
      } else {
        throw std::runtime_error(
          "Papyrus VM: null argument for Opcodes::op_PropGet");
      }
      break;
    case OpcodesImplementation::Opcodes::op_PropSet:
      if (args[1] != nullptr) {
        argsForCall.push_back(*args[2]);
        std::string nameProperty = (const char*)*args[0];
        auto object = static_cast<IGameObject*>(
          IsSelfStr(*args[1]) ? activeInstanceOwner : *args[1]);
        if (!object)
          object = static_cast<IGameObject*>(activeInstanceOwner);
        if (object && object->activePexInstances.size() > 0) {
          auto inst = object->activePexInstances.back();
          Object::PropInfo* runProperty =
            GetProperty(*inst, nameProperty, Object::PropInfo::kFlags_Write);
          if (runProperty != nullptr) {
            inst->StartFunction(runProperty->writeHandler, argsForCall,
                                ctx->stackIdHolder);
          }
        }
      } else {
        throw std::runtime_error(
          "Papyrus VM: null argument for Opcodes::op_PropSet");
      }
      break;
    case OpcodesImplementation::Opcodes::op_Array_Create:
      (*args[0]).pArray = std::make_shared<std::vector<VarValue>>();
      if ((int32_t)(*args[1]) > 0) {
        (*args[0]).pArray->resize((int32_t)(*args[1]));
        uint8_t type = GetArrayElementType((*args[0]).GetType());
        for (auto& element : *(*args[0]).pArray) {
          element = VarValue(type);
        }
      } else {
        throw std::runtime_error(
          "Papyrus VM: null argument for Opcodes::op_PropSet");
      }
      break;
    case OpcodesImplementation::Opcodes::op_Array_Length:
      if ((*args[1]).pArray != nullptr) {
        if ((*args[0]).GetType() == VarValue::kType_Integer) {
          *args[0] = VarValue((int32_t)(*args[1]).pArray->size());
        } else if ((*args[0]).GetType() == VarValue::kType_Float) {
          *args[0] = VarValue((double)(*args[1]).pArray->size());
        }
      } else {
        *args[0] = VarValue((int32_t)0);
      }
      break;
    case OpcodesImplementation::Opcodes::op_Array_GetElement:
      if ((*args[1]).pArray != nullptr) {
        *args[0] = (*args[1]).pArray->at((int32_t)(*args[2]));
      } else {
        *args[0] = VarValue::None();
      }
      break;
    case OpcodesImplementation::Opcodes::op_Array_SetElement:
      if ((*args[0]).pArray != nullptr) {
        (*args[0]).pArray->at((int32_t)(*args[1])) = *args[2];
      } else {
        throw std::runtime_error(
          "Papyrus VM: null argument for op_Array_SetElement opcode");
      }
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
    var.second = GetIndentifierValue(*locals, var.second);
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
      arg = &(GetIndentifierValue(*locals, *arg));
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
    int i = ctx.line - 1;
    size_t resultIdx =
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

VarValue& ActivePexInstance::GetIndentifierValue(
  std::vector<Local>& locals, VarValue& value, bool treatStringsAsIdentifiers)
{
  if (auto valueAsString = static_cast<const char*>(value)) {
    if (treatStringsAsIdentifiers &&
        value.GetType() == VarValue::kType_String) {
      auto& res = GetVariableValueByName(&locals, valueAsString);
      if (spdlog::should_log(spdlog::level::trace)) {
        spdlog::trace("GetIndentifierValue {}: {} = {}",
                      this->sourcePex.fn()->source, valueAsString,
                      res.ToString());
      }
      return res;
    }
    if (value.GetType() == VarValue::kType_Identifier) {
      auto& res = GetVariableValueByName(&locals, valueAsString);
      if (spdlog::should_log(spdlog::level::trace)) {
        spdlog::trace("GetIndentifierValue {}: {} = {}",
                      this->sourcePex.fn()->source, valueAsString,
                      res.ToString());
      }
      return res;
    }
  }
  return value;
}

uint8_t ActivePexInstance::GetTypeByName(std::string typeRef)
{

  std::transform(typeRef.begin(), typeRef.end(), typeRef.begin(), tolower);

  if (typeRef == "int") {
    return VarValue::kType_Integer;
  }
  if (typeRef == "float") {
    return VarValue::kType_Float;
  }
  if (typeRef == "string") {
    return VarValue::kType_String;
  }
  if (typeRef == "bool") {
    return VarValue::kType_Bool;
  }
  if (typeRef == "identifier") {
    assert(0);
  }
  if (typeRef == "string[]") {
    return VarValue::kType_StringArray;
  }
  if (typeRef == "int[]") {
    return VarValue::kType_IntArray;
  }
  if (typeRef == "float[]") {
    return VarValue::kType_FloatArray;
  }
  if (typeRef == "bool[]") {
    return VarValue::kType_BoolArray;
  }
  if (typeRef.find("[]") != std::string::npos) {
    return VarValue::kType_ObjectArray;
  }
  if (typeRef == "none") {
    // assert(false);
    return VarValue::kType_Object;
  }
  return VarValue::kType_Object;
}

uint8_t ActivePexInstance::GetArrayElementType(uint8_t type)
{
  uint8_t returnType;

  switch (type) {
    case VarValue::kType_ObjectArray:
      returnType = VarValue::kType_Object;

      break;
    case VarValue::kType_StringArray:
      returnType = VarValue::kType_String;

      break;
    case VarValue::kType_IntArray:
      returnType = VarValue::kType_Integer;

      break;
    case VarValue::kType_FloatArray:
      returnType = VarValue::kType_Float;

      break;
    case VarValue::kType_BoolArray:
      returnType = VarValue::kType_Bool;

      break;
    default:
      throw std::runtime_error(
        "Papyrus VM: Unable to get required type ::GetArrayElementType");
  }

  return returnType;
}

uint8_t ActivePexInstance::GetArrayTypeByElementType(uint8_t type)
{
  uint8_t returnType;

  switch (type) {
    case VarValue::kType_Object:
      returnType = VarValue::kType_ObjectArray;

      break;
    case VarValue::kType_String:
      returnType = VarValue::kType_StringArray;

      break;
    case VarValue::kType_Integer:
      returnType = VarValue::kType_IntArray;

      break;
    case VarValue::kType_Float:
      returnType = VarValue::kType_FloatArray;

      break;
    case VarValue::kType_Bool:
      returnType = VarValue::kType_BoolArray;

      break;
    default:
      throw std::runtime_error("Papyrus VM:  Unable to get required type "
                               "::GetArrayTypeByElementType");
  }

  return returnType;
}

void ActivePexInstance::CastObjectToObject(VarValue* result,
                                           VarValue* scriptToCastOwner,
                                           std::vector<Local>& locals)
{
  std::string objectToCastTypeName = scriptToCastOwner->objectType;
  const std::string& resultTypeName = result->objectType;

  if (scriptToCastOwner->GetType() != VarValue::kType_Object ||
      *scriptToCastOwner == VarValue::None()) {
    *result = VarValue::None();
    if (spdlog::should_log(spdlog::level::trace)) {
      spdlog::trace("CastObjectToObject {} -> {} (object is null)",
                    scriptToCastOwner->ToString(), result->ToString());
    }
    return;
  }

  std::vector<std::string> classesStack;

  auto object = static_cast<IGameObject*>(*scriptToCastOwner);
  if (object) {
    std::string scriptName = object->GetParentNativeScript();
    classesStack.push_back(scriptName);
    while (1) {
      if (scriptName.empty()) {
        break;
      }

      if (!Utils::stricmp(resultTypeName.data(), scriptName.data())) {
        *result = *scriptToCastOwner;
        if (spdlog::should_log(spdlog::level::trace)) {
          spdlog::trace("CastObjectToObject {} -> {} (match found: {})",
                        scriptToCastOwner->ToString(), result->ToString(),
                        resultTypeName);
        }
        return;
      }

      // TODO: Test this with attention
      // Here is the case when i.e. variable with type 'Form' casts to
      // 'ObjectReference' while it's actually an Actor

      auto myScriptPex = parentVM->GetPexByName(scriptName);

      if (!myScriptPex.fn) {
        spdlog::error("Script not found: {}", scriptName);
        break;
      }

      scriptName = myScriptPex.fn()->objectTable[0].parentClassName;
      classesStack.push_back(scriptName);
    }
  }

  *result = VarValue::None();
  if (spdlog::should_log(spdlog::level::trace)) {
    spdlog::trace(
      "CastObjectToObject {} -> {} (match not found, wanted {}, stack is {})",
      scriptToCastOwner->ToString(), result->ToString(), resultTypeName,
      fmt::join(classesStack, ", "));
  }
}

bool ActivePexInstance::HasParent(ActivePexInstance* script,
                                  std::string castToTypeName)
{

  if (script != nullptr) {

    if (script->sourcePex.fn()->source == castToTypeName)
      return true;

    if (script->parentInstance != nullptr &&
        script->parentInstance->sourcePex.fn()->source != "") {
      if (script->parentInstance->sourcePex.fn()->source == castToTypeName)
        return true;
      else
        return HasParent(script->parentInstance.get(), castToTypeName);
    }
  }

  return false;
}

bool ActivePexInstance::HasChild(ActivePexInstance* script,
                                 std::string castToTypeName)
{

  if (script != nullptr) {

    if (script->sourcePex.fn()->source == castToTypeName)
      return true;

    if (script->childrenName != "") {
      if (script->childrenName == castToTypeName)
        return true;
    }
  }
  return false;
}

VarValue& ActivePexInstance::GetVariableValueByName(std::vector<Local>* locals,
                                                    std::string name)
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
      if (auto var =
            variables->GetVariableByName(name.data(), *sourcePex.fn()))
        return *var;
  } catch (std::exception& e) {
    if (auto handler = parentVM->GetExceptionHandler()) {
      noneVar = VarValue::None();
      handler({ e.what(), sourcePex.fn()->source });
      return noneVar;
    } else {
      throw;
    }
  }

  for (auto& _name : identifiersValueNameCache) {
    if ((const char*)(*_name) == name) {
      return *_name;
    }
  }

  if (parentVM->IsNativeFunctionByNameExisted(GetSourcePexName())) {
    auto functionName = std::make_shared<VarValue>(name);

    identifiersValueNameCache.push_back(functionName);
    return *identifiersValueNameCache.back();
  }

  for (auto& _string : sourcePex.fn()->stringTable.GetStorage()) {
    if (_string == name) {
      auto stringTableValue = std::make_shared<VarValue>(name);

      identifiersValueNameCache.push_back(stringTableValue);
      return *identifiersValueNameCache.back();
    }
  }

  for (auto& _string :
       parentInstance->sourcePex.fn()->stringTable.GetStorage()) {
    if (_string == name) {
      auto stringTableParentValue = std::make_shared<VarValue>(name);

      identifiersValueNameCache.push_back(stringTableParentValue);
      return *identifiersValueNameCache.back();
    }
  }

  assert(false);
  static VarValue _;
  return _;
}
