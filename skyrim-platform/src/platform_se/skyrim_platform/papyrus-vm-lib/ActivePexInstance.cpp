#include "OpcodesImplementation.h"
#include "VirtualMachine.h"

ActivePexInstance::ActivePexInstance()
{
  this->parentVM = nullptr;
  this->sourcePex = nullptr;
}

ActivePexInstance::ActivePexInstance(std::shared_ptr<PexScript> sourcePex,
                                     VarForBuildActivePex mapForFillProperties,
                                     VirtualMachine* parentVM,
                                     VarValue activeInstanceOwner,
                                     std::string childrenName)
{
  this->childrenName = childrenName;
  this->activeInstanceOwner = activeInstanceOwner;
  this->parentVM = parentVM;
  this->sourcePex = sourcePex;
  this->parentInstance =
    FillParentInstance(sourcePex->objectTable.m_data[0].parentClassName,
                       activeInstanceOwner, mapForFillProperties);

  auto at = mapForFillProperties.find(sourcePex->source);

  std::vector<std::pair<std::string, VarValue>> argsForFillProperties;

  if (at != mapForFillProperties.end()) {
    argsForFillProperties = at->second;
  }

  variables = FillVariables(sourcePex, argsForFillProperties);
}

std::shared_ptr<ActivePexInstance> ActivePexInstance::FillParentInstance(
  std::string nameNeedScript, VarValue activeInstanceOwner,
  VarForBuildActivePex mapForFillProperties)
{
  for (auto& baseScript : parentVM->allLoadedScripts) {
    if (baseScript->source == nameNeedScript) {
      ActivePexInstance scriptInstance(baseScript, mapForFillProperties,
                                       parentVM, activeInstanceOwner,
                                       this->sourcePex->source);
      return std::make_shared<ActivePexInstance>(scriptInstance);
    }
  }

  if (nameNeedScript != "")
    throw std::runtime_error("nameNeedScript empty");
  return nullptr;
}

std::vector<ObjectTable::Object::VarInfo> ActivePexInstance::FillVariables(
  std::shared_ptr<PexScript> sourcePex,
  std::vector<std::pair<std::string, VarValue>> argsForFillProperties)
{
  std::vector<ObjectTable::Object::VarInfo> result;

  for (auto object : sourcePex->objectTable.m_data) {
    for (auto var : object.variables) {
      ObjectTable::Object::VarInfo varInfo;
      varInfo = var;
      if ((const char*)varInfo.value == nullptr) {
        varInfo.value = VarValue(GetTypeByName(var.typeName));
      }
      result.push_back(varInfo);
    }
  }

  // Creating temp variable for save State ActivePexInstance and
  // transition between them
  ObjectTable::Object::VarInfo variableForState = { "::State", "String", 0,
                                                    VarValue("") };

  result.push_back(variableForState);

  for (auto& object : sourcePex->objectTable.m_data) {
    for (auto& prop : object.properties) {
      for (auto var : argsForFillProperties) {
        if (prop.name == var.first) {
          for (auto& varInfo : result) {
            if (prop.autoVarName == varInfo.name) {
              varInfo.value = var.second;
              varInfo.value.objectType = varInfo.typeName;
              break;
            }
          }
        }
      }
    }
  }

  return result;
}

FunctionInfo ActivePexInstance::GetFunctionByName(const char* name,
                                                  std::string stateName)
{

  FunctionInfo function;
  for (auto& object : sourcePex->objectTable.m_data) {
    for (auto& state : object.states) {
      if (state.name == stateName) {
        for (auto& func : state.functions) {
          if (func.name == name) {
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

std::string ActivePexInstance::GetActiveStateName()
{
  std::string stateName;

  for (auto var : this->variables) {
    if (var.name == "::State") {
      stateName = (const char*)var.value;
      break;
    }
  }

  return stateName;
}

ObjectTable::Object::PropInfo* ActivePexInstance::GetProperty(
  ActivePexInstance* scriptInstance, std::string nameProperty, uint8_t flag)
{

  if (scriptInstance != nullptr) {

    if (flag == ObjectTable::Object::PropInfo::kFlags_Read) {

      for (auto& object : scriptInstance->sourcePex->objectTable.m_data) {
        for (auto& prop : object.properties) {
          if (prop.name == nameProperty &&
              (prop.flags & 5) == prop.kFlags_Read) {
            return &prop;
          }
        }
      }
    }

    if (flag == ObjectTable::Object::PropInfo::kFlags_Write) {

      for (auto& object : scriptInstance->sourcePex->objectTable.m_data) {
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

ActivePexInstance* ActivePexInstance::GetActivePexInObject(
  VarValue* object, std::string& scriptType)
{

  auto itObject = std::find_if(
    parentVM->gameObjects.begin(), parentVM->gameObjects.end(),
    [&](std::pair<std::shared_ptr<IGameObject>, std::vector<ActivePexInstance>>
          i) { return ((IGameObject*)object == i.first.get()); });

  if (itObject != parentVM->gameObjects.end()) {

    for (auto& instance : itObject->second) {
      if (instance.sourcePex->source == scriptType) {
        return &instance;
        break;
      }
    }
  }

  return nullptr;
}

VarValue ActivePexInstance::CastToString(const VarValue& var)
{
  std::string temp;
  size_t _size = this->sourcePex->stringTable.m_data.size();

  switch (var.GetType()) {

    case var.kType_Object: {
      IGameObject* ptr = ((IGameObject*)var);
      if (ptr) {
        return VarValue(ptr->GetStringID());
      } else {
        const static std::string noneString = "None";
        return VarValue(noneString.c_str());
      }
    }
    case var.kType_Identifier:
      throw std::runtime_error("Failed cast type_indentifier to String");
      return VarValue();

    case var.kType_Integer:

      temp = std::to_string((int)var);
      for (auto& str : this->sourcePex->stringTable.m_data) {
        if (str.data() == temp) {
          return VarValue(str.data());
        }
      }

      this->sourcePex->stringTable.m_data.push_back(temp);
      return VarValue(this->sourcePex->stringTable.m_data[_size].data());

    case var.kType_Float:

      temp = std::to_string((float)var);
      for (auto& str : this->sourcePex->stringTable.m_data) {
        if (str.data() == temp) {
          return VarValue(str.data());
        }
      }

      this->sourcePex->stringTable.m_data.push_back(temp);
      return VarValue(this->sourcePex->stringTable.m_data[_size].data());

    case var.kType_Bool: {

      const static std::string boolTrue = "True";
      const static std::string boolFalse = "False";

      temp = std::to_string((bool)var);
      if (temp == "0")
        return VarValue(boolFalse.c_str());
      else
        return VarValue(boolTrue.c_str());
    }
    case var.kType_ObjectArray:
      return GetElementsArrayAtString(var, var.kType_ObjectArray);

    case var.kType_StringArray:
      return GetElementsArrayAtString(var, var.kType_StringArray);

    case var.kType_IntArray:
      return GetElementsArrayAtString(var, var.kType_IntArray);

    case var.kType_FloatArray:
      return GetElementsArrayAtString(var, var.kType_FloatArray);

    case var.kType_BoolArray:
      return GetElementsArrayAtString(var, var.kType_BoolArray);

    default:
      assert(false);
      return VarValue();
  }
}

VarValue ActivePexInstance::GetElementsArrayAtString(const VarValue& array,
                                                     uint8_t type)
{
  std::string returnValue = "[";

  for (size_t i = 0; i < array.pArray->size(); ++i) {
    // returnValue += " ";

    switch (type) {
      case array.kType_ObjectArray:
        returnValue += ((IGameObject*)((*array.pArray)[i]))->GetStringID();
        break;

      case array.kType_StringArray:
        returnValue += (const char*)((*array.pArray)[i]);
        break;

      case array.kType_IntArray:
        returnValue += std::to_string((int)((*array.pArray)[i]));
        break;

      case array.kType_FloatArray:
        returnValue += std::to_string((float)((*array.pArray)[i]));
        break;

      case array.kType_BoolArray: {
        VarValue& temp = ((*array.pArray)[i]);
        returnValue += (const char*)(CastToString(temp));
        break;
      }
      default:
        throw std::runtime_error("Failed to get array of elements in string");
    }

    if (i < array.pArray->size() - 1)
      returnValue += " , ";
    else
      returnValue += "]";
  }

  instanceStringTable.push_back(std::make_shared<std::string>(returnValue));
  return VarValue(
    instanceStringTable[instanceStringTable.size() - 1]->c_str());
}

VarValue ActivePexInstance::StartFunction(FunctionInfo& function,
                                          std::vector<VarValue>& arguments)
{
  std::vector<std::pair<std::string, VarValue>> locals;

  bool needReturn = false;
  bool needJump = false;
  int jumpStep = 0;
  VarValue returnValue = VarValue::None();

  for (auto& var : function.locals) {
    VarValue temp = VarValue(GetTypeByName(var.type));
    temp.objectType = var.type;
    locals.push_back({ var.name, temp });
  }

  for (size_t i = 0; i < arguments.size(); ++i) {
    VarValue temp = arguments[i];
    temp.objectType = function.params[i].type;

    locals.push_back({ function.params[i].name, temp });
    assert(locals.back().second.GetType() == arguments[i].GetType());
  }

  for (size_t i = arguments.size(); i < function.params.size(); ++i) {
    const auto& var_ = function.params[i];

    VarValue temp = VarValue(GetTypeByName(var_.type));
    temp.objectType = var_.type;

    locals.push_back({ var_.name, temp });
  }

  for (auto& var : locals) {
    var.second = GetIndentifierValue(locals, var.second);
  }

  auto& sourceOpCode = function.code.instructions;

  std::vector<std::pair<uint8_t, std::vector<VarValue*>>> opCode;

  for (size_t i = 0; i < sourceOpCode.size(); ++i) {

    std::pair<uint8_t, std::vector<VarValue*>> temp;
    temp.first = sourceOpCode[i].op;

    for (size_t j = 0; j < sourceOpCode[i].args.size(); ++j) {
      temp.second.push_back(&sourceOpCode[i].args[j]);
    }
    opCode.push_back(temp);
  }

  for (auto& op : opCode) {

    for (auto& arg : op.second) {
      arg = &(GetIndentifierValue(locals, *arg));
    }
  }

  for (size_t line = 0; line < opCode.size(); ++line) {

    std::vector<VarValue> argsForCall;

    if (opCode[line].second.size() > 4) {
      for (size_t i = 4; i < opCode[line].second.size(); ++i) {
        argsForCall.push_back(*opCode[line].second[i]);
      }
    }

    if (opCode[line].first == FunctionCode::kOp_CallParent &&
        opCode[line].second.size() > 3) {
      for (size_t i = 3; i < opCode[line].second.size(); ++i) {
        argsForCall.push_back(*opCode[line].second[i]);
      }
    }

    switch (opCode[line].first) {
      case OpcodesImplementation::Opcodes::op_Nop:
        break;

      case OpcodesImplementation::Opcodes::op_iAdd:

        *opCode[line].second[0] =
          *opCode[line].second[1] + (*opCode[line].second[2]);
        break;

      case OpcodesImplementation::Opcodes::op_fAdd:

        *opCode[line].second[0] =
          *opCode[line].second[1] + (*opCode[line].second[2]);
        break;

      case OpcodesImplementation::Opcodes::op_iSub:

        *opCode[line].second[0] =
          *opCode[line].second[1] - (*opCode[line].second[2]);
        break;

      case OpcodesImplementation::Opcodes::op_fSub:

        *opCode[line].second[0] =
          *opCode[line].second[1] - (*opCode[line].second[2]);
        break;

      case OpcodesImplementation::Opcodes::op_iMul:

        *opCode[line].second[0] =
          *opCode[line].second[1] * (*opCode[line].second[2]);
        break;

      case OpcodesImplementation::Opcodes::op_fMul:

        *opCode[line].second[0] =
          *opCode[line].second[1] * (*opCode[line].second[2]);
        break;

      case OpcodesImplementation::Opcodes::op_iDiv:

        *opCode[line].second[0] =
          *opCode[line].second[1] / (*opCode[line].second[2]);
        break;

      case OpcodesImplementation::Opcodes::op_fDiv:

        *opCode[line].second[0] =
          *opCode[line].second[1] / (*opCode[line].second[2]);
        break;

      case OpcodesImplementation::Opcodes::op_iMod:

        *opCode[line].second[0] =
          *opCode[line].second[1] % (*opCode[line].second[2]);
        break;

      case OpcodesImplementation::Opcodes::op_Not:

        *opCode[line].second[0] = !(*opCode[line].second[1]);
        break;

      case OpcodesImplementation::Opcodes::op_iNeg:

        *opCode[line].second[0] = *opCode[line].second[1] * VarValue(-1);
        break;

      case OpcodesImplementation::Opcodes::op_fNeg:

        *opCode[line].second[0] = *opCode[line].second[1] * VarValue(-1.0f);
        break;

      case OpcodesImplementation::Opcodes::op_Assign:

        *opCode[line].second[0] = *opCode[line].second[1];
        break;

      case OpcodesImplementation::Opcodes::op_Cast:

        switch ((*opCode[line].second[0]).GetType()) {
          case VarValue::kType_Object:

            CastObjectToObject(opCode[line].second[0], opCode[line].second[1],
                               locals);
            break;
          case VarValue::kType_Integer:
            *opCode[line].second[0] = (*opCode[line].second[1]).CastToInt();
            break;
          case VarValue::kType_Float:
            *opCode[line].second[0] = (*opCode[line].second[1]).CastToFloat();
            break;
          case VarValue::kType_Bool:
            *opCode[line].second[0] = (*opCode[line].second[1]).CastToBool();
            break;
          case VarValue::kType_String:
            *opCode[line].second[0] = CastToString(*opCode[line].second[1]);
            break;
          default:
            assert(0);
        }

        break;

      case OpcodesImplementation::op_Cmp_eq:

        *opCode[line].second[0] =
          VarValue((*opCode[line].second[1]) == (*opCode[line].second[2]));
        break;

      case OpcodesImplementation::Opcodes::op_Cmp_lt:

        *opCode[line].second[0] =
          VarValue(*opCode[line].second[1] < *opCode[line].second[2]);
        break;

      case OpcodesImplementation::Opcodes::op_Cmp_le:

        *opCode[line].second[0] =
          VarValue(*opCode[line].second[1] <= *opCode[line].second[2]);
        break;

      case OpcodesImplementation::Opcodes::op_Cmp_gt:

        *opCode[line].second[0] =
          VarValue(*opCode[line].second[1] > *opCode[line].second[2]);
        break;

      case OpcodesImplementation::Opcodes::op_Cmp_ge:

        *opCode[line].second[0] =
          VarValue(*opCode[line].second[1] >= *opCode[line].second[2]);
        break;

      case OpcodesImplementation::Opcodes::op_Jmp:

        jumpStep = (int)(*opCode[line].second[0]) - 1;

        needJump = true;
        break;

      case OpcodesImplementation::Opcodes::op_Jmpt:

        if ((bool)(*opCode[line].second[0])) {
          jumpStep = (int)(*opCode[line].second[1]) - 1;

          needJump = true;
        }
        break;

      case OpcodesImplementation::Opcodes::op_Jmpf:

        if ((bool)(!(*opCode[line].second[0]))) {
          jumpStep = (int)(*opCode[line].second[1]) - 1;
          needJump = true;
        }
        break;

      case OpcodesImplementation::Opcodes::op_CallMethod: {

        IGameObject* object = (IGameObject*)(*opCode[line].second[1]);
        std::string functionName = (const char*)(*opCode[line].second[0]);

        static const std::string nameOnBeginState = "onBeginState";
        static const std::string nameOnEndState = "onEndState";

        if (functionName == nameOnBeginState ||
            functionName == nameOnEndState) {
          parentVM->SendEvent(this, functionName.c_str(), argsForCall);
          break;
        } else {
          *opCode[line].second[2] = parentVM->CallMethod(
            this, (IGameObject*)object, functionName.c_str(), argsForCall);
        }
      }

      break;

      case OpcodesImplementation::Opcodes::op_CallParent:

        *opCode[line].second[1] = parentVM->CallMethod(
          parentInstance.get(), (IGameObject*)activeInstanceOwner,
          (const char*)(*opCode[line].second[0]), argsForCall);

        break;

      case OpcodesImplementation::Opcodes::op_CallStatic: {

        const char* className = (const char*)(*opCode[line].second[0]);
        const char* functionName = (const char*)(*opCode[line].second[1]);

        *opCode[line].second[2] =
          parentVM->CallStatic(className, functionName, argsForCall);
      } break;

      case OpcodesImplementation::Opcodes::op_Return:

        returnValue = *opCode[line].second[0];
        needReturn = true;
        break;

      case OpcodesImplementation::Opcodes::op_StrCat:

        OpcodesImplementation::strCat(
          *opCode[line].second[0], *opCode[line].second[1],
          *opCode[line].second[2], this->sourcePex->stringTable);
        break;
      case OpcodesImplementation::Opcodes::op_PropGet:

        if (opCode[line].second[1] != nullptr) {

          std::string nameProperty = (const char*)*opCode[line].second[0];

          ActivePexInstance* ptrPex = GetActivePexInObject(
            opCode[line].second[1], opCode[line].second[1]->objectType);

          ObjectTable::Object::PropInfo* runProperty = GetProperty(
            ptrPex, nameProperty, ObjectTable::Object::PropInfo::kFlags_Read);

          if (runProperty != nullptr) {
            *opCode[line].second[2] =
              ptrPex->StartFunction(runProperty->readHandler, argsForCall);
          }

        } else {
          throw std::runtime_error(
            "(if) did not pass the test in case Opcodes::Op_PropGet");
        }

        break;

      case OpcodesImplementation::Opcodes::op_PropSet:

        if (opCode[line].second[1] != nullptr) {
          argsForCall.push_back(*opCode[line].second[2]);

          std::string nameProperty = (const char*)*opCode[line].second[0];

          ActivePexInstance* ptrPex = GetActivePexInObject(
            opCode[line].second[1], opCode[line].second[1]->objectType);

          ObjectTable::Object::PropInfo* runProperty = GetProperty(
            ptrPex, nameProperty, ObjectTable::Object::PropInfo::kFlags_Write);

          if (runProperty != nullptr) {
            ptrPex->StartFunction(runProperty->writeHandler, argsForCall);
          }

        } else {
          throw std::runtime_error(
            "(if) did not pass the test in case Opcodes::Op_PropSet");
        }
        break;

      case OpcodesImplementation::Opcodes::op_Array_Create:

        (*opCode[line].second[0]).pArray =
          std::shared_ptr<std::vector<VarValue>>(new std::vector<VarValue>);
        if ((int32_t)(*opCode[line].second[1]) > 0) {

          (*opCode[line].second[0])
            .pArray->resize((int32_t)(*opCode[line].second[1]));

          uint8_t type =
            GetArrayElementType((*opCode[line].second[0]).GetType());

          for (auto& element : *(*opCode[line].second[0]).pArray) {
            element = VarValue(type);
          }

        } else {
          throw std::runtime_error(
            "(if) did not pass the test in case Opcodes::Op_Array_Create");
        }

        break;

      case OpcodesImplementation::Opcodes::op_Array_Length:

        if ((*opCode[line].second[1]).pArray != nullptr) {
          if ((*opCode[line].second[0]).GetType() == VarValue::kType_Integer)
            *opCode[line].second[0] =
              VarValue((int32_t)(*opCode[line].second[1]).pArray->size());
        } else {
          *opCode[line].second[0] = VarValue((int32_t)0);
        }
        break;

      case OpcodesImplementation::Opcodes::op_Array_GetElement:

        if ((*opCode[line].second[1]).pArray != nullptr) {
          *opCode[line].second[0] =
            (*opCode[line].second[1])
              .pArray->at((int32_t)(*opCode[line].second[2]));
        } else {
          assert(0);
          throw std::runtime_error("(if) did not pass the test in case "
                                   "Opcodes::Op_Array_Get_Element");
        }

        break;

      case OpcodesImplementation::Opcodes::op_Array_SetElement:

        if ((*opCode[line].second[0]).pArray != nullptr) {
          (*opCode[line].second[0])
            .pArray->at((int32_t)(*opCode[line].second[1])) =
            *opCode[line].second[2];
        } else {
          throw std::runtime_error(
            "(if) did not pass the test in case Opcodes::Op_Array_SetElement");
        }

        break;

      case OpcodesImplementation::Opcodes::op_Array_FindElement:

        OpcodesImplementation::arrayFindElement(
          *opCode[line].second[0], *opCode[line].second[1],
          *opCode[line].second[2], *opCode[line].second[3]);
        break;

      case OpcodesImplementation::Opcodes::op_Array_RfindElement:

        OpcodesImplementation::arrayRFindElement(
          *opCode[line].second[0], *opCode[line].second[1],
          *opCode[line].second[2], *opCode[line].second[3]);
        break;

      default:
        assert(0);
    }

    if (needReturn) {
      needReturn = false;
      return returnValue;
    }

    if (needJump) {
      needJump = false;
      line += jumpStep;
    }
  }
  return returnValue;
}

VarValue& ActivePexInstance::GetIndentifierValue(
  std::vector<std::pair<std::string, VarValue>>& locals, VarValue& value)
{
  if (value.GetType() == VarValue::kType_Identifier &&
      (const char*)value != nullptr) {
    std::string temp;
    temp = temp + (const char*)value;
    return GetVariableValueByName(locals, temp);
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
    throw std::runtime_error("TypeRef equals indetifier,GetTypeByName()");
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
      assert(false);
  }

  return returnType;
}

void ActivePexInstance::CastObjectToObject(
  VarValue* result, VarValue* scriptToCastOwner,
  std::vector<std::pair<std::string, VarValue>>& locals)
{
  std::string objectToCastTypeName = scriptToCastOwner->objectType;
  std::string resultTypeName = result->objectType;

  for (auto& var : locals) {
    if (resultTypeName == var.first) {
      resultTypeName = var.second.objectType;
      break;
    }
  }

  if (resultTypeName != "" && objectToCastTypeName != "") {
    auto scriptOwner = std::find_if(
      parentVM->gameObjects.begin(), parentVM->gameObjects.end(),
      [&](
        std::pair<std::shared_ptr<IGameObject>, std::vector<ActivePexInstance>>
          i) { return ((IGameObject*)*scriptToCastOwner) == i.first.get(); });

    ActivePexInstance* ptrScriptToCast = nullptr;

    if (scriptOwner != parentVM->gameObjects.end()) {
      for (auto& activeScript : scriptOwner->second) {
        if (activeScript.sourcePex->source == objectToCastTypeName) {
          ptrScriptToCast = &activeScript;
          break;
        }
      }
    }

    if (HasParent(ptrScriptToCast, resultTypeName) ||
        HasChild(ptrScriptToCast, resultTypeName)) {
      *result = *scriptToCastOwner;
      return;
    } else {
      *result = VarValue::None();
    }
  } else {
    *result = VarValue::None();
  }
}

bool ActivePexInstance::HasParent(ActivePexInstance* script,
                                  std::string castToTypeName)
{

  if (script != nullptr) {
    if (script->sourcePex->source == castToTypeName)
      return true;

    if (script->parentInstance != nullptr &&
        script->parentInstance->sourcePex->source != "") {
      if (script->parentInstance->sourcePex->source == castToTypeName)
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

    if (script->sourcePex->source == castToTypeName)
      return true;

    if (script->childrenName != "") {
      if (script->childrenName == castToTypeName)
        return true;
    }
  }
  return false;
}

VarValue& ActivePexInstance::GetVariableValueByName(
  std::vector<std::pair<std::string, VarValue>>& locals, std::string name)
{

  if (name == "self") {
    return activeInstanceOwner;
  }

  for (auto& var : locals) {
    if (var.first == name) {
      return var.second;
    }
  }

  for (auto& var : this->variables) {
    if (var.name == name) {
      return var.value;
    }
  }

  for (auto& _name : identifiersValueNameCache) {
    if ((const char*)(*_name) == name) {
      return *_name;
    }
  }

  auto at = parentVM->nativeFunctions.find(this->sourcePex->source);

  if (at != parentVM->nativeFunctions.end()) {
    for (auto& func : at->second) {
      if (func.first == name) {
        identifiersValueNameCache.push_back(
          std::make_shared<VarValue>(func.first.c_str()));
        return *identifiersValueNameCache[(identifiersValueNameCache.size() -
                                           1)];
      }
    }
  }

  auto it = parentVM->nativeStaticFunctions.find(name);

  if (it != parentVM->nativeStaticFunctions.end()) {
    identifiersValueNameCache.push_back(
      std::make_shared<VarValue>(it->first.c_str()));
    return *identifiersValueNameCache[(identifiersValueNameCache.size() - 1)];
  }

  for (auto& _string : sourcePex->stringTable.m_data) {
    if (_string == name) {
      identifiersValueNameCache.push_back(
        std::make_shared<VarValue>(_string.c_str()));
      return *identifiersValueNameCache[(identifiersValueNameCache.size() -
                                         1)];
    }
  }

  for (auto& _string : parentInstance->sourcePex->stringTable.m_data) {
    if (_string == name) {
      identifiersValueNameCache.push_back(
        std::make_shared<VarValue>(_string.c_str()));
      return *identifiersValueNameCache[(identifiersValueNameCache.size() -
                                         1)];
    }
  }

  assert(false);
  static VarValue _;
  return _;
}