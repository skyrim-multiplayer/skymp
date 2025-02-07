#include "ScopedTask.h"
#include "antigo/Context.h"
#include "antigo/ResolvedContext.h"
#include "papyrus-vm/OpcodesImplementation.h"
#include "papyrus-vm/Utils.h"
#include "papyrus-vm/VarValue.h"
#include "papyrus-vm/VirtualMachine.h"
#include <algorithm>
#include <cctype> // tolower
#include <chrono>
#include <exception>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <functional>
#include <memory>
#include <optional>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <string>

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
  ANTIGO_CONTEXT_INIT(ctx);

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
  const ActivePexInstance& scriptInstance, std::string propertyName,
  uint8_t flag)
{
  if (!scriptInstance.IsValid())
    return nullptr;

  if (flag == Object::PropInfo::kFlags_Read) {

    for (auto& object : scriptInstance.sourcePex.fn()->objectTable) {
      for (auto& prop : object.properties) {
        if (prop.name == propertyName &&
            (prop.flags & 5) == prop.kFlags_Read) {
          return &prop;
        }
      }
    }

    if (flag == Object::PropInfo::kFlags_Write) {

      for (auto& object : scriptInstance.sourcePex.fn()->objectTable) {
        for (auto& prop : object.properties) {
          if (prop.name == propertyName &&
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

struct ActivePexInstance::ExecutionContext
{
  std::shared_ptr<StackData> stackData;
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
  ANTIGO_CONTEXT_INIT(agctx);

  if (!callResult.promise) {
    agctx.AddMessage("call result is not a promise");
    return true;
  }

  agctx.AddMessage("call result is not a promise (fixed 20250206)");

  std::optional<Antigo::ResolvedContext> agctxResolved;
  if (ctx->stackData->tracing.enabled) {
    ctx->stackData->tracing.msgs.push_back("processing promise return value: yield");
    agctxResolved = agctx.Resolve();
  }

  Viet::Promise<VarValue> currentFnPr;

  auto ctxCopy = *ctx;
  callResult.promise->Then([this, ctxCopy, currentFnPr, agctxPrev = std::move(agctxResolved)](VarValue v) {
    ANTIGO_CONTEXT_INIT(agctx);
    agctx.AddLambdaWithOwned([agctxPrev = std::move(agctxPrev)]() -> std::string {
      if (agctxPrev) {
        return agctxPrev->ToString();
      }
      return "tracing disabled, no prev context";
    });

    auto ctxCopy_ = ctxCopy;
    ctxCopy_.line++;
    if (ctxCopy_.stackData->tracing.enabled) {
      ctxCopy_.stackData->tracing.msgs.push_back("processing promise return value: continue");
    }
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
  ANTIGO_CONTEXT_INIT(agctx);
  agctx.AddMessage("next: op, args.size(), args");
  agctx.AddUnsigned(op);
  agctx.AddUnsigned(args.size());
  // XXX bad naming, really it means outer scope
  agctx.AddLambdaWithOwned([&args]() {
    std::stringstream ss;
    ss << "size = " << args.size() << "\n[\n";
    for (const auto& arg : args) {
      ss << "  " << (arg ? arg->ToString() : "(null)") << "\n";
    }
    ss << "]";
    return std::move(ss).str();
  });

  auto argsForCall = GetArgsForCall(op, args);

  switch (op) {
    case OpcodesImplementation::Opcodes::op_Nop:
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back("nop");
      }
      break;
    case OpcodesImplementation::Opcodes::op_iAdd:
    case OpcodesImplementation::Opcodes::op_fAdd:
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back("add");
      }
      *args[0] = *args[1] + (*args[2]);
      break;
    case OpcodesImplementation::Opcodes::op_iSub:
    case OpcodesImplementation::Opcodes::op_fSub:
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back("sub");
      }
      *args[0] = *args[1] - (*args[2]);
      break;
    case OpcodesImplementation::Opcodes::op_iMul:
    case OpcodesImplementation::Opcodes::op_fMul:
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back("mul");
      }
      *args[0] = *args[1] * (*args[2]);
      break;
    case OpcodesImplementation::Opcodes::op_iDiv:
    case OpcodesImplementation::Opcodes::op_fDiv:
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back("div");
      }
      *args[0] = *args[1] / (*args[2]);
      break;
    case OpcodesImplementation::Opcodes::op_iMod:
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back("mod");
      }
      *args[0] = *args[1] % (*args[2]);
      break;
    case OpcodesImplementation::Opcodes::op_Not:
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back("not");
      }
      *args[0] = !(*args[1]);
      break;
    case OpcodesImplementation::Opcodes::op_iNeg:
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back("neg");
      }
      *args[0] = *args[1] * VarValue(-1);
      break;
    case OpcodesImplementation::Opcodes::op_fNeg:
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back("neg");
      }
      *args[0] = *args[1] * VarValue(-1.0f);
      break;
    case OpcodesImplementation::Opcodes::op_Assign:
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back(fmt::format("assign {} = {}", args[0]->ToString(), args[1]->ToString()));
      }
      *args[0] = *args[1];
      break;
    case OpcodesImplementation::Opcodes::op_Cast:
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back(fmt::format("cast {} = {}", args[0]->ToString(), args[1]->ToString()));
      }
      switch ((*args[0]).GetType()) {
        case VarValue::kType_Object: {
          auto to = args[0];
          auto from = IsSelfStr(*args[1]) ? &activeInstanceOwner : args[1];
          CastObjectToObject(*parentVM, to, from);
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
          *args[0] = VarValue::CastToString(*args[1]);
          break;
        default:
          // assert(0);
          // Triggered by some array stuff in SkyMP, not sure this is OK
          agctx.AddMessage("strange cast, had comment \"Triggered by some array stuff in SkyMP, not sure this is OK 4 years ago\" :)");
          agctx.Orphan();
          *args[0] = (*args[1]);
          break;
      }
      break;
    case OpcodesImplementation::op_Cmp_eq:
      *args[0] = VarValue((*args[1]) == (*args[2]));
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back(fmt::format(
          "cmp {} == {}: {}", args[1]->ToString(), args[2]->ToString(), args[0]->ToString()
        ));
      }
      break;
    case OpcodesImplementation::Opcodes::op_Cmp_lt:
      *args[0] = VarValue(*args[1] < *args[2]);
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back(fmt::format(
          "cmp {} < {}: {}", args[1]->ToString(), args[2]->ToString(), args[0]->ToString()
        ));
      }
      break;
    case OpcodesImplementation::Opcodes::op_Cmp_le:
      *args[0] = VarValue(*args[1] <= *args[2]);
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back(fmt::format(
          "cmp {} <= {}: {}", args[1]->ToString(), args[2]->ToString(), args[0]->ToString()
        ));
      }
      break;
    case OpcodesImplementation::Opcodes::op_Cmp_gt:
      *args[0] = VarValue(*args[1] > *args[2]);
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back(fmt::format(
          "cmp {} > {}: {}", args[1]->ToString(), args[2]->ToString(), args[0]->ToString()
        ));
      }
      break;
    case OpcodesImplementation::Opcodes::op_Cmp_ge:
      *args[0] = VarValue(*args[1] >= *args[2]);
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back(fmt::format(
          "cmp {} >= {}: {}", args[1]->ToString(), args[2]->ToString(), args[0]->ToString()
        ));
      }
      break;
    case OpcodesImplementation::Opcodes::op_Jmp:
      ctx->jumpStep = (int)(*args[0]) - 1;
      ctx->needJump = true;
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back(fmt::format(
          "jmp"
        ));
      }
      break;
    case OpcodesImplementation::Opcodes::op_Jmpt:
      if ((bool)(*args[0])) {
        ctx->jumpStep = (int)(*args[1]) - 1;
        ctx->needJump = true;
      }
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back(fmt::format(
          "jmpt {}", args[0]->ToString()
        ));
      }
      break;
    case OpcodesImplementation::Opcodes::op_Jmpf:
      if ((bool)(!(*args[0]))) {
        ctx->jumpStep = (int)(*args[1]) - 1;
        ctx->needJump = true;
      }
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back(fmt::format(
          "jmpf {}", args[0]->ToString()
        ));
      }
      break;
    case OpcodesImplementation::Opcodes::op_CallParent: {
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back(fmt::format(
          "callparent"
        ));
      }

      auto parentName =
        parentInstance ? parentInstance->GetSourcePexName() : "";
      try {
        auto gameObject = static_cast<IGameObject*>(activeInstanceOwner);

        std::vector<std::shared_ptr<ActivePexInstance>>
          activePexInstancesForCallParent;
        if (gameObject) {
          activePexInstancesForCallParent =
            gameObject->ListActivePexInstances();

          std::string toFind = sourcePex.source;

          for (auto& v : activePexInstancesForCallParent) {
            if (!Utils::stricmp(v->GetSourcePexName().data(), toFind.data())) {
              v = parentInstance;
              spdlog::trace("CallParent: redirecting method call {} -> {}",
                            toFind, parentName);
              if (ctx->stackData->tracing.enabled) {
                ctx->stackData->tracing.msgs.push_back(fmt::format(
                  "CallParent: redirecting method call {} -> {}",
                            toFind, parentName
                ));
              }
            }
          }
        }

        if (spdlog::should_log(spdlog::level::trace)) {
          std::vector<std::string> argsForCallStr;
          for (auto v : argsForCall) {
            argsForCallStr.push_back(v.ToString());
          }
          spdlog::trace("CallParent: calling with args {}",
                        fmt::join(argsForCallStr, ", "));
        }

        auto res = parentVM->CallMethod(gameObject, (const char*)(*args[0]),
                                        argsForCall, ctx->stackData,
                                        &activePexInstancesForCallParent);
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
        throw std::runtime_error("Anomaly in CallMethod. String expected");

      std::string functionName = (const char*)(*args[0]);

      if (ctx->stackData->tracing.enabled) {
        std::string vars;
        for (const auto& arg : argsForCall) {
          vars += " " + arg.ToString();
        }
        ctx->stackData->tracing.msgs.push_back(fmt::format("callmethod {}.{}{}", object->ToString(), functionName, vars));
      }

      static const std::string nameOnBeginState = "onBeginState";
      static const std::string nameOnEndState = "onEndState";
      try {
        if (functionName == nameOnBeginState ||
            functionName == nameOnEndState) {
          // TODO: consider using CallMethod here. I'm afraid that this event
          // will pollute other scripts attached to an object
          parentVM->SendEvent(this, functionName.c_str(), argsForCall);
          if (ctx->stackData->tracing.enabled) {
            ctx->stackData->tracing.msgs.push_back(fmt::format("callmethod {}.{}: sent as event", object->ToString(), functionName));
          }
          break;
        } else {
          auto nullableGameObject = static_cast<IGameObject*>(*object);
          auto res =
            parentVM->CallMethod(nullableGameObject, functionName.c_str(),
                                 argsForCall, ctx->stackData);
          spdlog::trace("callmethod object={} funcName={} result={}",
                        object->ToString(), functionName, res.ToString());
          if (ctx->stackData->tracing.enabled) {
            ctx->stackData->tracing.msgs.push_back(fmt::format("callmethod {}.{}: result {}", object->ToString(), functionName, res.ToString()));
          }
          if (EnsureCallResultIsSynchronous(res, ctx)) {
            *args[2] = res;
          }
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
        if (ctx->stackData->tracing.enabled) {
          std::string argsStr;
          for (const auto& arg : argsForCall) {
            argsStr += " " + arg.ToString();
          }
          ctx->stackData->tracing.msgs.push_back(fmt::format("callstatic {}.{}{}", className, functionName, argsStr));
        }
        auto res = parentVM->CallStatic(className, functionName, argsForCall,
                                        ctx->stackData);
        if (ctx->stackData->tracing.enabled) {
          ctx->stackData->tracing.msgs.push_back(fmt::format("callstatic {}.{}: result {}", className, functionName, res.ToString()));
        }
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
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back(fmt::format("return {}", args[0]->ToString()));
      }

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
      if (args[0] == nullptr) {
        spdlog::error(
          "Papyrus VM: null argument with index 0 for Opcodes::op_PropGet");
      } else if (args[1] == nullptr) {
        spdlog::error(
          "Papyrus VM: null argument with index 1 for Opcodes::op_PropGet");
      } else if (args[2] == nullptr) {
        spdlog::error(
          "Papyrus VM: null argument with index 2 for Opcodes::op_PropGet");
      } else {
        if (ctx->stackData->tracing.enabled) {
          ctx->stackData->tracing.msgs.push_back(fmt::format("propget {} {}", args[0]->ToString(), args[1]->ToString()));
        }

        std::string propertyName;

        if (args[0]->GetType() == VarValue::kType_String ||
            args[0]->GetType() == VarValue::kType_Identifier) {
          propertyName = static_cast<const char*>(*args[0]);
        } else {
          spdlog::error("Papyrus VM: argument with index 0 has unexpected "
                        "type {} for Opcodes::op_PropGet",
                        static_cast<int>(args[0]->GetType()));
        }

        IGameObject* object = nullptr;
        VarValue objectVarValue =
          IsSelfStr(*args[1]) ? activeInstanceOwner : *args[1];
        if (objectVarValue.GetType() == VarValue::kType_Object) {
          object = static_cast<IGameObject*>(objectVarValue);
        } else {
          spdlog::error("Papyrus VM: object has unexpected type {} for "
                        "Opcodes::op_PropGet",
                        static_cast<int>(objectVarValue.GetType()));
        }

        if (!object) {
          if (activeInstanceOwner.GetType() == VarValue::kType_Object) {
            object = static_cast<IGameObject*>(activeInstanceOwner);
          } else {
            spdlog::error(
              "Papyrus VM: activeInstanceOwner has unexpected type {} for "
              "Opcodes::op_PropGet",
              static_cast<int>(activeInstanceOwner.GetType()));
          }
        }

        if (object && object->ListActivePexInstances().size() > 0) {
          auto inst = object->ListActivePexInstances().back();
          agctx.AddMessage("next: inst, runProperty");
          agctx.AddPtr(inst);
          Object::PropInfo* runProperty =
            GetProperty(*inst, propertyName, Object::PropInfo::kFlags_Read);
          agctx.AddPtr(runProperty);
          if (runProperty != nullptr) {
            // TODO: use of argsForCall looks incorrect. why use argsForCall
            // here? shoud be {} (empty args)
            *args[2] = inst->StartFunction(runProperty->readHandler,
                                           argsForCall, ctx->stackData);
            spdlog::trace("propget function called");
          } else {
            auto& instProps = inst->sourcePex.fn()->objectTable[0].properties;
            auto it =
              std::find_if(instProps.begin(), instProps.end(),
                           [&](const Object::PropInfo& propInfo) {
                             return !Utils::stricmp(propInfo.name.data(),
                                                    propertyName.data());
                           });
            if (it == instProps.end()) {
              spdlog::trace("propget do nothing: prop {} not found",
                            propertyName);
            } else {
              VarValue* var = inst->variables->GetVariableByName(
                it->autoVarName.data(), *inst->sourcePex.fn());
              if (var) {
                *args[2] = *var;
              } else {
                spdlog::trace("propget do nothing: variable {} not found",
                              it->autoVarName);
              }
            }
          }
          spdlog::trace("propget propName={} object={} result={}",
                        args[0]->ToString(), args[1]->ToString(),
                        args[2]->ToString());
          if (ctx->stackData->tracing.enabled) {
            ctx->stackData->tracing.msgs.push_back(fmt::format("propget {} {}: result {}", args[0]->ToString(), args[1]->ToString(), args[2]->ToString()));
          }
        }
      }
      break;
    case OpcodesImplementation::Opcodes::op_PropSet:
      if (args[0] == nullptr) {
        spdlog::error(
          "Papyrus VM: null argument with index 0 for Opcodes::op_PropSet");
      } else if (args[1] == nullptr) {
        spdlog::error(
          "Papyrus VM: null argument with index 1 for Opcodes::op_PropSet");
      } else if (args[2] == nullptr) {
        spdlog::error(
          "Papyrus VM: null argument with index 2 for Opcodes::op_PropSet");
      } else {
        if (ctx->stackData->tracing.enabled) {
          ctx->stackData->tracing.msgs.push_back(fmt::format("propset {} {} {}", args[0]->ToString(), args[1]->ToString(), args[2]->ToString()));
        }

        // TODO: use of argsForCall looks incorrect
        argsForCall.push_back(*args[2]);

        std::string propertyName;

        if (args[0]->GetType() == VarValue::kType_String ||
            args[0]->GetType() == VarValue::kType_Identifier) {
          propertyName = static_cast<const char*>(*args[0]);
        } else {
          spdlog::error("Papyrus VM: argument with index 0 has unexpected "
                        "type {} for Opcodes::op_PropSet",
                        static_cast<int>(args[0]->GetType()));
        }

        IGameObject* object = nullptr;
        VarValue objectVarValue =
          IsSelfStr(*args[1]) ? activeInstanceOwner : *args[1];
        if (objectVarValue.GetType() == VarValue::kType_Object) {
          object = static_cast<IGameObject*>(objectVarValue);
        } else {
          spdlog::error("Papyrus VM: object has unexpected type {} for "
                        "Opcodes::op_PropSet",
                        static_cast<int>(objectVarValue.GetType()));
        }

        if (!object) {
          if (activeInstanceOwner.GetType() == VarValue::kType_Object) {
            object = static_cast<IGameObject*>(activeInstanceOwner);
          } else {
            spdlog::error(
              "Papyrus VM: activeInstanceOwner has unexpected type {} for "
              "Opcodes::op_PropSet",
              static_cast<int>(activeInstanceOwner.GetType()));
          }
        }

        if (object && object->ListActivePexInstances().size() > 0) {
          agctx.AddMessage("next: inst, runProperty");
          auto inst = object->ListActivePexInstances().back();
          agctx.AddPtr(inst);
          Object::PropInfo* runProperty =
            GetProperty(*inst, propertyName, Object::PropInfo::kFlags_Write);
          agctx.AddPtr(runProperty);
          if (runProperty != nullptr) {
            // TODO: use of argsForCall looks incorrect.
            // probably should only *args[2]
            inst->StartFunction(runProperty->writeHandler, argsForCall,
                                ctx->stackData);
            spdlog::trace("propset function called");
          } else {
            auto& instProps = inst->sourcePex.fn()->objectTable[0].properties;
            auto it =
              std::find_if(instProps.begin(), instProps.end(),
                           [&](const Object::PropInfo& propInfo) {
                             return !Utils::stricmp(propInfo.name.data(),
                                                    propertyName.data());
                           });
            if (it == instProps.end()) {
              spdlog::trace("propset do nothing: prop {} not found",
                            propertyName);
            } else {
              VarValue* var = inst->variables->GetVariableByName(
                it->autoVarName.data(), *inst->sourcePex.fn());
              if (var) {
                *var = *args[2];
              } else {
                spdlog::trace("propset do nothing: variable {} not found",
                              it->autoVarName);
              }
            }
          }
          spdlog::trace("propset propName={} object={} result={}",
                        args[0]->ToString(), args[1]->ToString(),
                        args[2]->ToString());
        }
      }
      break;
    case OpcodesImplementation::Opcodes::op_Array_Create:
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back("array create");
      }
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
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back(fmt::format("array len &{} := len {}", args[0]->ToString(), args[1]->ToString()));
      }
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
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back(fmt::format("array get {}[{}]: result {}", args[1]->ToString(), args[2]->ToString(), args[0]->ToString()));
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
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back("array findelement");
      }
      OpcodesImplementation::ArrayFindElement(*args[0], *args[1], *args[2],
                                              *args[3]);
      break;
    case OpcodesImplementation::Opcodes::op_Array_RfindElement:
      if (ctx->stackData->tracing.enabled) {
        ctx->stackData->tracing.msgs.push_back("array rfindelement");
      }
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
        // Do not dereference functionName
        dereferenceStart = 1;
        break;
      case OpcodesImplementation::Opcodes::op_PropGet:
      case OpcodesImplementation::Opcodes::op_PropSet:
        // Do not dereference property name
        dereferenceStart = 1;
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

namespace {
void TracingInit(std::shared_ptr<StackData> stackData, Antigo::OnstackContext& agctxParent) {
  size_t prevSize = stackData->tracing.msgs.size();
  // size_t start = 0;
  // if (prevSize > 5) {
  //   start = prevSize - 5;
  // }
  if (prevSize > 5) {
    stackData->tracing.msgs.clear();
  }
  stackData->tracing.msgs.push_back(fmt::format("{} msgs before", prevSize));

  spdlog::info("TRACING PAPYRUS STACK {}-{}: TracingInit (detected enable)", stackData->stackIdHolder.GetStackId(), stackData->tracing.traceId);
  agctxParent.AddLambdaWithOwned([stackData]{
    return fmt::format("TRACING PAPYRUS STACK {}-{}", stackData->stackIdHolder.GetStackId(), stackData->tracing.traceId);
  });
  agctxParent.AddLambdaWithOwned([stackData] {
    std::stringstream ss;
    ss << fmt::format("stack tracing for {}-{} [{}]:", stackData->stackIdHolder.GetStackId(), stackData->tracing.traceId, stackData->tracing.msgs.size()) << "\n";
    for (const auto& msg : stackData->tracing.msgs) {
      ss << msg << "\n";
    }
    return std::move(ss).str();
  });
  agctxParent.LogInnerExecution();
}

void TracingResolve(std::shared_ptr<StackData> stackData, const VarValue& val) {
  ANTIGO_CONTEXT_INIT(agctx);

  auto g = agctx.AddLambdaWithRef([&val]{return "return = " + val.ToString();});
  g.Arm();
  spdlog::info("TRACING PAPYRUS STACK {}-{}: {}", stackData->stackIdHolder.GetStackId(), stackData->tracing.traceId, agctx.Resolve().ToString());
}

bool PexNameIsTraced(const std::string& pexName) {
  if (pexName.starts_with("SweetMineWood")) {
    return true;
  }
  if (pexName.starts_with("SweetMineOre")) {
    return true;
  }
  if (pexName.starts_with("SweetClothesRemoveList")) { // tutorial book
    return true;
  }
  if (pexName.starts_with("SweetMineIngot")) { // smith
    return true;
  }
  return false;
}
} // namespace

VarValue ActivePexInstance::ExecuteAll(
  ExecutionContext& ctx, std::optional<VarValue> previousCallResult)
{
  ANTIGO_CONTEXT_INIT(agctx);

  agctx.AddMessage("next: line");
  agctx.AddUnsigned(ctx.line);
  auto g = agctx.AddLambdaWithRef([&ctx]() {
    std::stringstream ss;
    ss << "ExecutionContext:\n";
    if (ctx.locals == nullptr) {
      ss << "locals = nullptr\n";
    } else {
      ss << "locals = [" << ctx.locals->size() << "] [\n";
      for (size_t i = 0; i < ctx.locals->size(); ++i) {
        auto& local = (*ctx.locals)[i];
        ss << "  " << local.first << " = " << local.second << "\n";
      }
      ss << "]\n";
    }
    return std::move(ss).str();
  });
  g.Arm();

  if (ctx.stackData == nullptr) {
    spdlog::error("ActivePexInstance::ExecuteAll: stackData is null\n{}", agctx.Resolve().ToString());
    return VarValue::None();
  }

  agctx.AddMessage("next: stack id");
  agctx.AddUnsigned(ctx.stackData->stackIdHolder.GetStackId());

  auto pipex = sourcePex.fn(); // XXX ???

  if (pipex != nullptr && PexNameIsTraced(pipex->source)) {
    ctx.stackData->EnableTracing(agctx);
  }

  struct Defer {
    ExecutionContext& ctx;

    ~Defer() {
      ctx.stackData->lastExec = std::chrono::steady_clock::now();
    }
  } defer{ctx};

  bool tracingStartedHere = false;
  if (ctx.stackData->tracing.enabled && !agctx.IsLoggingInnerExecution()) {
    TracingInit(ctx.stackData, agctx);
    tracingStartedHere = true;
  }

  auto g2 = agctx.AddLambdaWithRef([&pipex]() {
    std::stringstream ss;
    ss << "pipex->?source = ";
    if (pipex) {
      ss << pipex->source;
    } else {
      ss << "(nullptr)";
    }
    return std::move(ss).str();
  });
  g2.Arm();

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
      if (tracingStartedHere) {
        TracingResolve(ctx.stackData, ctx.returnValue);
      }
      return ctx.returnValue;
    }

    if (ctx.needJump) {
      ctx.needJump = false;
      ctx.line += ctx.jumpStep;
    }
  }
  auto result = ctx.returnValue;
  if (tracingStartedHere) {
    TracingResolve(ctx.stackData, result);
  }
  return result;
}

VarValue ActivePexInstance::StartFunction(FunctionInfo& function,
                                          std::vector<VarValue>& arguments,
                                          std::shared_ptr<StackData> stackData)
{
  ANTIGO_CONTEXT_INIT(agctx);

  if (!stackData) {
    throw std::runtime_error("An empty stackData passed to StartFunction");
  }

  // XXX bad naming, outer scope
  agctx.AddLambdaWithOwned([this, &function]() {
    std::stringstream ss;
    ss << "source pex = " << GetSourcePexName() << "\n";
    ss << "return = " << function.returnType << "\n";
    ss << "docstring = " << function.docstring << "\n";
    ss << "userFlags = " << std::hex << function.userFlags << "\n";
    ss << "flags = " << std::hex << static_cast<int>(function.flags) << "\n";
    ss << "params: [\n";
    for (const auto& param : function.params) {
      ss << "  " << param.name << " " << param.type << "\n";
    }
    ss << "]\n";
    ss << "locals:\n";
    for (const auto& param : function.locals) {
      ss << "  " << param.name << " " << param.type << "\n";
    }
    ss << "]\n";
    return std::move(ss).str();
  });

  if (PexNameIsTraced(GetSourcePexName())) {
    stackData->EnableTracing(agctx);
  }

  bool tracingStartedHere = false;
  if (stackData->tracing.enabled && !agctx.IsLoggingInnerExecution()) {
    TracingInit(stackData, agctx);
    tracingStartedHere = true;
  }

  thread_local StackDepthHolder g_stackDepthHolder;

  g_stackDepthHolder.IncreaseStackDepth();

  Viet::ScopedTask<StackDepthHolder> stackDepthDecreaseTask(
    [](StackDepthHolder& stackDepthHolder) {
      stackDepthHolder.DecreaseStackDepth();
    },
    g_stackDepthHolder);

  constexpr size_t kMaxStackDepth = 128;

  if (g_stackDepthHolder.GetStackDepth() >= kMaxStackDepth) {
    spdlog::error("ActivePexInstance::StartFunction - Stack overflow in "
                  "script {}, returning None",
                  sourcePex.fn()->source);
    agctx.Resolve().Print();
    return VarValue::None();
  }

  auto locals = MakeLocals(function, arguments);
  ExecutionContext ctx{ stackData, function.code.instructions, locals };
  auto result = ExecuteAll(ctx);
  if (tracingStartedHere) {
    TracingResolve(stackData, result);
    // XXX or add to the vector?
  }
  return result;
}

VarValue& ActivePexInstance::GetIndentifierValue(
  std::vector<Local>& locals, VarValue& value, bool treatStringsAsIdentifiers)
{
  if (auto valueAsString = static_cast<const char*>(value)) {
    if (treatStringsAsIdentifiers &&
        value.GetType() == VarValue::kType_String) {
      auto& res = GetVariableValueByName(&locals, valueAsString);
      return res;
    }
    if (value.GetType() == VarValue::kType_Identifier) {
      auto& res = GetVariableValueByName(&locals, valueAsString);
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

void ActivePexInstance::CastObjectToObject(const VirtualMachine& vm,
                                           VarValue* result,
                                           VarValue* scriptToCastOwner)
{
  static const VarValue kNone = VarValue::None();

  if (scriptToCastOwner->GetType() != VarValue::kType_Object) {
    *result = kNone;
    return spdlog::trace(
      "CastObjectToObject {} -> {} (object is not an object)",
      scriptToCastOwner->ToString(), result->ToString());
  }

  if (*scriptToCastOwner == kNone) {
    *result = kNone;
    return spdlog::trace("CastObjectToObject {} -> {} (object is None)",
                         scriptToCastOwner->ToString(), result->ToString());
  }

  const std::string& resultTypeName = result->objectType;

  VarValue tmp;
  std::vector<std::string> outClassesStack;

  if (tmp == kNone) {
    tmp = TryCastToBaseClass(vm, resultTypeName, scriptToCastOwner,
                             outClassesStack);
    if (tmp != kNone) {
      spdlog::trace("CastObjectToObject {} -> {} (base class found: {})",
                    scriptToCastOwner->ToString(), tmp.ToString(),
                    resultTypeName);
    }
  }

  if (tmp == kNone) {
    tmp = TryCastMultipleInheritance(vm, resultTypeName, scriptToCastOwner);
    if (tmp != kNone) {
      spdlog::trace(
        "CastObjectToObject {} -> {} (multiple inheritance found: {})",
        scriptToCastOwner->ToString(), tmp.ToString(), resultTypeName);
    }
  }

  if (tmp == kNone) {
    spdlog::trace(
      "CastObjectToObject {} -> {} (match not found, wanted {}, stack is {})",
      scriptToCastOwner->ToString(), tmp.ToString(), resultTypeName,
      fmt::join(outClassesStack, ", "));
  }

  *result = tmp;
}

VarValue ActivePexInstance::TryCastToBaseClass(
  const VirtualMachine& vm, const std::string& resultTypeName,
  VarValue* scriptToCastOwner, std::vector<std::string>& outClassesStack)
{
  ANTIGO_CONTEXT_INIT(ctx);

  auto object = static_cast<IGameObject*>(*scriptToCastOwner);
  if (!object) {
    return VarValue::None();
  }

  std::string scriptName = object->GetParentNativeScript();
  outClassesStack.push_back(scriptName);
  while (true) {
    if (scriptName.empty()) {
      break;
    }

    if (!Utils::stricmp(resultTypeName.data(), scriptName.data())) {
      return *scriptToCastOwner;
    }

    // TODO: Test this with attention
    // Here is the case when i.e. variable with type 'Form' casts to
    // 'ObjectReference' while it's actually an Actor

    auto myScriptPex = vm.GetPexByName(scriptName);

    if (!myScriptPex.fn) {
      spdlog::error("Script not found: {}", scriptName);
      break;
    }

    scriptName = myScriptPex.fn()->objectTable[0].parentClassName;
    outClassesStack.push_back(scriptName);
  }

  return VarValue::None();
}

VarValue ActivePexInstance::TryCastMultipleInheritance(
  const VirtualMachine& vm, const std::string& resultTypeName,
  VarValue* scriptToCastOwner)
{
  auto object = static_cast<IGameObject*>(*scriptToCastOwner);
  if (!object) {
    return VarValue::None();
  }

  // TODO: support cast to base class in parallel inheritance chain
  // i.e. X extends Y, in script Z we cast smth to X while it is Y
  // Current code would fail in this case. I guess it'd be better to find such
  // cases in game, then implement.
  for (auto& script : object->ListActivePexInstances()) {
    if (Utils::stricmp(script->GetSourcePexName().data(),
                       resultTypeName.data()) == 0) {
      return script->activeInstanceOwner;
    }
  }

  return VarValue::None();
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
  ANTIGO_CONTEXT_INIT(agctx);

  if (name == "self") {
    agctx.AddMessage("returning self");
    return activeInstanceOwner;
  }

  if (locals)
    for (auto& var : *locals) {
      if (var.first == name) {
        agctx.AddMessage("returning from locals");
        return var.second;
      }
    }

  try {
    if (variables) {
      if (auto var =
            variables->GetVariableByName(name.data(), *sourcePex.fn())) {
        agctx.AddMessage("returning from variables");
        return *var;
      }
    }
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
      agctx.AddMessage("returning from identifiersValueNameCache path 1");
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
      agctx.AddMessage("returning from identifiersValueNameCache path 2");
      return *identifiersValueNameCache.back();
    }
  }

  for (auto& _string :
       parentInstance->sourcePex.fn()->stringTable.GetStorage()) {
    if (_string == name) {
      auto stringTableParentValue = std::make_shared<VarValue>(name);

      identifiersValueNameCache.push_back(stringTableParentValue);
      agctx.AddMessage("returning from identifiersValueNameCache path 3");
      return *identifiersValueNameCache.back();
    }
  }

  agctx.AddMessage("assert false reached");
  spdlog::critical("assert false reached: {}", agctx.Resolve().ToString());

  assert(false);
  static VarValue _;
  return _;
}
