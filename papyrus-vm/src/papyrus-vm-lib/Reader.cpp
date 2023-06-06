#include "papyrus-vm/Reader.h"
#include <fstream>

void Reader::Read()
{
  arrayBytes.clear();

  std::ifstream file(path, std::ios::binary);

  if (file.is_open()) {

    file.seekg(0, std::ios_base::end);
    const std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios_base::beg);
    arrayBytes.reserve(fileSize);

    char temp;
    while (file.get(temp)) {
      arrayBytes.push_back(temp);
    }

  } else {
    throw std::runtime_error("Error open file: " + path);
  }

  file.close();
};

std::vector<std::shared_ptr<PexScript>> Reader::GetSourceStructures()
{
  return sourceStructures;
}

Reader::Reader(const std::vector<std::string>& vectorPath)
{
  for (auto path : vectorPath) {
    this->currentReadPositionInFile = 0;
    this->path = path;
    Read();
    CreateScriptStructure(arrayBytes);
  }
}

Reader::Reader(const std::vector<std::vector<uint8_t>>& pexVector)
{
  for (auto& pex : pexVector)
    CreateScriptStructure(pex);
}

void Reader::CreateScriptStructure(const std::vector<uint8_t>& arrayBytes)
{
  this->arrayBytes = arrayBytes;
  currentReadPositionInFile = 0;

  this->structure = std::make_shared<PexScript>();

  FillHeader(structure->header);

  FillSource(structure->source);
  FillUser(structure->user);
  FillMachine(structure->machine);

  FillStringTable(structure->stringTable);
  FillDebugInfo(structure->debugInfo);
  FillUserFlagTable(structure->userFlagTable);
  FillObjectTable(structure->objectTable);
  sourceStructures.push_back(structure);
}

void Reader::FillHeader(ScriptHeader& scriptHeader)
{
  scriptHeader.Signature = Read32_bit(); // 00	FA57C0DE
  scriptHeader.VerMajor = Read8_bit();   // 04	03
  scriptHeader.VerMinor = Read8_bit();   // 05	01
  scriptHeader.GameID = Read16_bit();    // 06	0001
  scriptHeader.BuildTime = Read64_bit(); // 08	time_t
}

void Reader::FillSource(std::string& str)
{
  int sizeString = Read16_bit();
  str = ReadString(sizeString);

  for (int i = 0; i < 4; ++i) {
    str.pop_back();
  }
}

void Reader::FillUser(std::string& str)
{
  int sizeString = Read16_bit();
  str = ReadString(sizeString);
}

void Reader::FillMachine(std::string& str)
{
  int sizeString = Read16_bit();
  str = ReadString(sizeString);
}

void Reader::FillStringTable(StringTable& strTable)
{
  std::vector<std::string> storage;

  int stringTableSize = Read16_bit();
  storage.reserve(stringTableSize);

  for (int i = 0; i < stringTableSize; i++) {
    int sizeString = Read16_bit();
    storage.push_back(ReadString(sizeString));
  }

  strTable.SetStorage(storage);
}

void Reader::FillDebugInfo(DebugInfo& debugInfo)
{
  debugInfo.m_flags = Read8_bit();
  debugInfo.m_sourceModificationTime = Read64_bit();

  int functionCount = Read16_bit();
  debugInfo.m_data.reserve(functionCount);

  for (int i = 0; i < functionCount; i++) {
    debugInfo.m_data.push_back(FillDebugFunction());
  }
}

DebugInfo::DebugFunction Reader::FillDebugFunction()
{
  DebugInfo::DebugFunction Fdebug;

  Fdebug.objName = this->structure->stringTable.GetStorage()[Read16_bit()];
  Fdebug.stateName = this->structure->stringTable.GetStorage()[Read16_bit()];
  Fdebug.fnName = this->structure->stringTable.GetStorage()[Read16_bit()];
  Fdebug.type = Read8_bit();

  int instrunctionCount = Read16_bit();
  for (int i = 0; i < instrunctionCount; i++) {
    Fdebug.lineNumbers.push_back(Read16_bit());
  }

  return Fdebug;
}

void Reader::FillUserFlagTable(std::vector<UserFlag>& userFlagTable)
{
  int userFlagCount = Read16_bit();

  for (int i = 0; i < userFlagCount; i++) {
    userFlagTable.push_back(FillUserFlag());
  }
}

UserFlag Reader::FillUserFlag()
{
  UserFlag flag;

  flag.name = this->structure->stringTable.GetStorage()[Read16_bit()];
  flag.idx = Read8_bit();

  return flag;
}

void Reader::FillObjectTable(std::vector<Object>& objectTable)
{
  int objectCount = Read16_bit();
  objectTable.reserve(objectCount);

  for (int i = 0; i < objectCount; i++) {
    objectTable.push_back(FillObject());
  }
}

Object Reader::FillObject()
{
  Object object;

  object.NameIndex = this->structure->stringTable.GetStorage()[Read16_bit()];

  Read32_bit(); //	Dont remove!   size includes itself for some reason,
                // hence size-4

  object.parentClassName =
    this->structure->stringTable.GetStorage()[Read16_bit()];
  object.docstring = this->structure->stringTable.GetStorage()[Read16_bit()];
  object.userFlags = Read32_bit();
  object.autoStateName =
    this->structure->stringTable.GetStorage()[Read16_bit()];

  int numVariables = Read16_bit();
  object.variables.reserve(numVariables);

  for (int i = 0; i < numVariables; i++) {
    object.variables.push_back(FillVariable());
  }

  int numProperties = Read16_bit();
  object.properties.reserve(numProperties);

  for (int i = 0; i < numProperties; i++) {
    object.properties.push_back(FillProperty());
  }

  int numStates = Read16_bit();
  object.states.reserve(numStates);

  for (int i = 0; i < numStates; i++) {
    object.states.push_back(FillState());
  }
  return object;
}

Object::VarInfo Reader::FillVariable()
{
  Object::VarInfo Var;

  Var.name = this->structure->stringTable.GetStorage()[Read16_bit()];
  Var.typeName = this->structure->stringTable.GetStorage()[Read16_bit()];
  Var.userFlags = Read32_bit();
  Var.value = FillVariableData();

  return Var;
}

VarValue Reader::FillVariableData()
{
  VarValue Data;

  uint8_t type = Read8_bit();

  switch (type) {
    case Data.kType_Object:
      Data = VarValue::None();
      break;
    case Data.kType_Identifier:
      Data = VarValue(
        Data.kType_Identifier,
        this->structure->stringTable.GetStorage()[Read16_bit()].data());
      break;
    case Data.kType_String:
      Data = VarValue(
        this->structure->stringTable.GetStorage()[Read16_bit()].data());
      break;
    case Data.kType_Integer:
      Data = VarValue((int32_t)Read32_bit());
      break;
    case Data.kType_Float: {
      uint32_t v = Read32_bit();
      Data = VarValue(*(float*)&v);
      // Data.data.f = (int)Read32_bit();
    } break;
    case Data.kType_Bool:
      Data = VarValue((bool)Read8_bit());
      break;
    case VarValue::kType_ObjectArray:
      Read32_bit();
      break;
    case VarValue::kType_StringArray:
      Read32_bit();
      break;
    case VarValue::kType_IntArray:
      Read32_bit();
      break;
    case VarValue::kType_FloatArray:
      Read32_bit();
      break;
    case VarValue::kType_BoolArray:
      Read32_bit();
      break;
    default:
      throw std::runtime_error(
        "Papyrus VM: Failed to populate variables with data");
  }

  return Data;
}

FunctionInfo Reader::FillFuncInfo()
{
  FunctionInfo info;

  info.returnType = this->structure->stringTable.GetStorage()[Read16_bit()];
  info.docstring = this->structure->stringTable.GetStorage()[Read16_bit()];
  info.userFlags = Read32_bit();
  info.flags = Read8_bit();

  int countParams = Read16_bit();
  info.params.reserve(countParams);

  for (int i = 0; i < countParams; i++) {
    FunctionInfo::ParamInfo temp;
    temp.name = this->structure->stringTable.GetStorage()[Read16_bit()];
    temp.type = this->structure->stringTable.GetStorage()[Read16_bit()];
    info.params.push_back(temp);
  }

  int countLocals = Read16_bit();
  info.params.reserve(countLocals);

  for (int i = 0; i < countLocals; i++) {
    FunctionInfo::ParamInfo temp;
    temp.name = this->structure->stringTable.GetStorage()[Read16_bit()];
    temp.type = this->structure->stringTable.GetStorage()[Read16_bit()];
    info.params.push_back(temp);
  }

  int countInstructions = Read16_bit();

  info.code = FillFunctionCode(countInstructions);

  return info;
}

FunctionCode Reader::FillFunctionCode(int countInstructions)
{
  FunctionCode funcCode;
  funcCode.instructions.reserve(countInstructions);

  for (int i = 0; i < countInstructions; i++) {
    FunctionCode::Instruction item;
    item.op = Read8_bit();

    int numArguments = GetCountArguments(item.op);
    item.args.reserve(numArguments);

    for (int i = 0; i < numArguments; i++) {

      item.args.push_back(FillVariableData());

      if (i == numArguments - 1) {
        if (additionalArguments) {
          int numAdditionalArgs = (int)item.args[i];
          for (int i = 0; i < numAdditionalArgs; ++i) {
            item.args.push_back(FillVariableData());
          }
        }
        additionalArguments = false;
      }
    };

    funcCode.instructions.push_back(item);
  };
  return funcCode;
}

uint8_t Reader::GetCountArguments(uint8_t opcode)
{

  int count = numArgumentsForOpcodes[opcode];

  if (count < 0) {
    additionalArguments = true;
    count *= -1;
  }

  return count;
}

Object::PropInfo Reader::FillProperty()
{
  Object::PropInfo prop;

  prop.name = this->structure->stringTable.GetStorage()[Read16_bit()];
  prop.type = this->structure->stringTable.GetStorage()[Read16_bit()];
  prop.docstring = this->structure->stringTable.GetStorage()[Read16_bit()];
  prop.userFlags = Read32_bit();
  prop.flags = Read8_bit();

  if ((prop.flags & 4) == prop.kFlags_AutoVar) { // it exists??
    prop.autoVarName = this->structure->stringTable.GetStorage()[Read16_bit()];
  }

  if ((prop.flags & 5) == prop.kFlags_Read) {
    prop.readHandler = FillFuncInfo();
  }

  if ((prop.flags & 6) == prop.kFlags_Write) {
    prop.writeHandler = FillFuncInfo();
  }

  return prop;
}

Object::StateInfo Reader::FillState()
{
  Object::StateInfo stateinfo;

  stateinfo.name = this->structure->stringTable.GetStorage()[Read16_bit()];

  int numFunctions = Read16_bit();

  for (int i = 0; i < numFunctions; ++i) {
    stateinfo.functions.push_back(FillStateFunction());
  }

  return stateinfo;
}

Object::StateInfo::StateFunction Reader::FillStateFunction()
{
  Object::StateInfo::StateFunction temp;

  temp.name = this->structure->stringTable.GetStorage()[Read16_bit()];
  temp.function = FillFuncInfo();

  return temp;
}

uint8_t Reader::Read8_bit()
{
  uint8_t temp;

  temp = arrayBytes[currentReadPositionInFile];
  currentReadPositionInFile++;

  return temp;
}

uint16_t Reader::Read16_bit()
{
  uint16_t temp = 0;

  for (int i = 0; i < 2; i++) {
    temp = temp * 256 + arrayBytes[currentReadPositionInFile];
    currentReadPositionInFile++;
  }

  return temp;
}

uint32_t Reader::Read32_bit()
{
  uint32_t temp = 0;

  for (int i = 0; i < 4; i++) {
    temp = temp * 256 + arrayBytes[currentReadPositionInFile];
    currentReadPositionInFile++;
  }
  return temp;
}

uint64_t Reader::Read64_bit()
{
  uint64_t temp = 0;

  for (int i = 0; i < 8; i++) {
    temp = temp * 256 + arrayBytes[currentReadPositionInFile];
    currentReadPositionInFile++;
  }
  return temp;
}

std::string Reader::ReadString(int size)
{
  std::string temp;
  temp.reserve(size);

  for (int i = 0; i < size; i++) {
    temp += (char)arrayBytes[currentReadPositionInFile];
    currentReadPositionInFile++;
  }

  return temp;
}
