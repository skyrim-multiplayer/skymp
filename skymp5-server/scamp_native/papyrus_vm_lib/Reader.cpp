#include "Reader.h"
#include <fstream>

void Reader::Read()
{

  char temp;

  arrayBytes.clear();

  std::ifstream File(path, std::ios::binary);

  if (File.is_open()) {

    File.seekg(0, std::ios_base::end);
    File.seekg(0, std::ios_base::beg);

    while (File.get(temp)) {
      arrayBytes.push_back(temp);
    }

  } else {
    throw std::runtime_error("Error open file: " + path);
  }

  File.close();
};

std::vector<std::shared_ptr<PexScript>> Reader::GetSourceStructures()
{
  return sourceStructures;
}

Reader::Reader(std::vector<std::string> vectorPath)
{
  for (auto path : vectorPath) {
    this->currentReadPositionInFile = 0;
    this->path = path;
    Read();
    CreateScriptStructure(arrayBytes);
  }
}

Reader::Reader(std::vector<std::vector<uint8_t>> pexVector)
{
  for (auto& pex : pexVector)
    CreateScriptStructure(pex);
}

void Reader::CreateScriptStructure(std::vector<uint8_t> arrayBytes)
{
  this->arrayBytes = arrayBytes;
  currentReadPositionInFile = 0;

  this->structure = std::make_shared<PexScript>();

  structure->header = FillHeader();

  structure->source = FillSource();
  structure->user = FillUser();
  structure->machine = FillMachine();

  structure->stringTable = FillStringTable();
  structure->debugInfo = FillDebugInfo();
  structure->userFlagTable = FillUserFlagTable();
  structure->objectTable = FillObjectTable();
  sourceStructures.push_back(structure);
}

ScriptHeader Reader::FillHeader()
{
  ScriptHeader Header;

  Header.Signature = Read32_bit(); // 00	FA57C0DE
  Header.VerMajor = Read8_bit();   // 04	03
  Header.VerMinor = Read8_bit();   // 05	01
  Header.GameID = Read16_bit();    // 06	0001
  Header.BuildTime = Read64_bit(); // 08	time_t

  return Header;
}

std::string Reader::FillSource()
{
  std::string source;
  int SizeString = Read16_bit();
  source = ReadString(SizeString);

  for (int i = 0; i < 4; ++i) {
    source.pop_back();
  }

  return source;
}

std::string Reader::FillUser()
{
  std::string user;

  int SizeString = Read16_bit();
  user = ReadString(SizeString);

  return user;
}

std::string Reader::FillMachine()
{
  std::string machine;

  int SizeString = Read16_bit();
  machine = ReadString(SizeString);

  return machine;
}

StringTable Reader::FillStringTable()
{
  std::vector<std::string> storage;

  int SizeStringTable = Read16_bit();

  for (int i = 0; i < SizeStringTable; i++) {
    int SizeString = Read16_bit();
    storage.push_back(ReadString(SizeString));
  }

  StringTable stringTable;
  stringTable.SetStorage(storage);
  return stringTable;
}

DebugInfo Reader::FillDebugInfo()
{
  DebugInfo debugInfo;

  debugInfo.m_flags = Read8_bit();
  debugInfo.m_sourceModificationTime = Read64_bit();

  int FunctionCount = Read16_bit();

  for (int i = 0; i < FunctionCount; i++) {
    DebugInfo::DebugFunction info;
    info = FillDebugFunction();
    debugInfo.m_data.push_back(info);
  }

  return debugInfo;
}

DebugInfo::DebugFunction Reader::FillDebugFunction()
{
  DebugInfo::DebugFunction Fdebug;

  Fdebug.objName = this->structure->stringTable.GetStorage()[Read16_bit()];
  Fdebug.stateName = this->structure->stringTable.GetStorage()[Read16_bit()];
  Fdebug.fnName = this->structure->stringTable.GetStorage()[Read16_bit()];
  Fdebug.type = Read8_bit();

  int InstrunctionCount = Read16_bit();
  for (int i = 0; i < InstrunctionCount; i++) {
    Fdebug.lineNumbers.push_back(Read16_bit());
  }

  return Fdebug;
}

UserFlagTable Reader::FillUserFlagTable()
{
  UserFlagTable userFlagTable;

  int UserFlagCount = Read16_bit();

  for (int i = 0; i < UserFlagCount; i++) {
    UserFlagTable::UserFlag flag;
    flag = FillUserFlag();
    userFlagTable.m_data.push_back(flag);
  }

  return userFlagTable;
}

UserFlagTable::UserFlag Reader::FillUserFlag()
{
  UserFlagTable::UserFlag flag;

  flag.name = this->structure->stringTable.GetStorage()[Read16_bit()];
  flag.idx = Read8_bit();

  return flag;
}

ObjectTable Reader::FillObjectTable()
{
  ObjectTable objectTable;

  int ObjectCount = Read16_bit();

  for (int i = 0; i < ObjectCount; i++) {
    ObjectTable::Object object;
    object = FillObject();
    objectTable.m_data.push_back(object);
  }

  return objectTable;
}

ObjectTable::Object Reader::FillObject()
{
  ObjectTable::Object object;

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

  for (int i = 0; i < numVariables; i++) {
    object.variables.push_back(FillVariable());
  }

  int numProperties = Read16_bit();

  for (int i = 0; i < numProperties; i++) {
    object.properties.push_back(FillProperty());
  }

  int numStates = Read16_bit();

  for (int i = 0; i < numStates; i++) {
    object.states.push_back(FillState());
  }

  return object;
}

ObjectTable::Object::VarInfo Reader::FillVariable()
{
  ObjectTable::Object::VarInfo Var;

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
      assert(false);
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

  int CountParams = Read16_bit();

  for (int i = 0; i < CountParams; i++) {
    FunctionInfo::ParamInfo temp;
    temp.name = this->structure->stringTable.GetStorage()[Read16_bit()];
    temp.type = this->structure->stringTable.GetStorage()[Read16_bit()];
    info.params.push_back(temp);
  }

  int CountLocals = Read16_bit();

  for (int i = 0; i < CountLocals; i++) {
    FunctionInfo::ParamInfo temp;
    temp.name = this->structure->stringTable.GetStorage()[Read16_bit()];
    temp.type = this->structure->stringTable.GetStorage()[Read16_bit()];
    info.params.push_back(temp);
  }

  int CountInstructions = Read16_bit();

  info.code = FillFunctionCode(CountInstructions);

  return info;
}

FunctionCode Reader::FillFunctionCode(int CountInstructions)
{
  FunctionCode funcCode;
  for (int i = 0; i < CountInstructions; i++) {
    FunctionCode::Instruction item;
    item.op = Read8_bit();

    int numArguments = GetCountArguments(item.op);

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

ObjectTable::Object::PropInfo Reader::FillProperty()
{
  ObjectTable::Object::PropInfo prop;

  prop.name = this->structure->stringTable.GetStorage()[Read16_bit()];
  prop.type = this->structure->stringTable.GetStorage()[Read16_bit()];
  prop.docstring = this->structure->stringTable.GetStorage()[Read16_bit()];
  prop.userFlags = Read32_bit();
  prop.flags = Read8_bit();

  if ((prop.flags & 4) == prop.kFlags_AutoVar) { // it exists??
    prop.autoVarName = this->structure->stringTable.GetStorage()[Read16_bit()];
  } else
    prop.autoVarName;

  if ((prop.flags & 5) == prop.kFlags_Read) {
    prop.readHandler = FillFuncInfo();
  } else
    prop.readHandler;

  if ((prop.flags & 6) == prop.kFlags_Write) {
    prop.writeHandler = FillFuncInfo();
  } else
    prop.writeHandler;

  return prop;
}

ObjectTable::Object::StateInfo Reader::FillState()
{
  ObjectTable::Object::StateInfo stateinfo;

  stateinfo.name = this->structure->stringTable.GetStorage()[Read16_bit()];

  int numFunctions = Read16_bit();

  for (int i = 0; i < numFunctions; ++i) {
    stateinfo.functions.push_back(FillStateFunction());
  }

  return stateinfo;
}

ObjectTable::Object::StateInfo::StateFunction Reader::FillStateFunction()
{
  ObjectTable::Object::StateInfo::StateFunction temp;

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
  uint16_t temp = NULL;

  for (int i = 0; i < 2; i++) {
    temp = temp * 256 + arrayBytes[currentReadPositionInFile];
    currentReadPositionInFile++;
  }

  return temp;
}

uint32_t Reader::Read32_bit()
{
  uint32_t temp = NULL;

  for (int i = 0; i < 4; i++) {
    temp = temp * 256 + arrayBytes[currentReadPositionInFile];
    currentReadPositionInFile++;
  }
  return temp;
}

uint64_t Reader::Read64_bit()
{
  uint64_t temp = NULL;

  for (int i = 0; i < 8; i++) {
    temp = temp * 256 + arrayBytes[currentReadPositionInFile];
    currentReadPositionInFile++;
  }
  return temp;
}

std::string Reader::ReadString(int Size)
{
  std::string temp = "";

  for (int i = 0; i < Size; i++) {
    temp += (char)arrayBytes[currentReadPositionInFile];
    currentReadPositionInFile++;
  }

  return temp;
}
