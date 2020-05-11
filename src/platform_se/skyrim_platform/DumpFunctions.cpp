#include "DumpFunctions.h"

#include "GetNativeFunctionAddr.h"
#include "NullPointerException.h"
#include "papyrus-vm-lib/Reader.h"
#include <RE/BSScript/IFunction.h>
#include <RE/BSScript/Internal/VirtualMachine.h>
#include <RE/BSScript/ObjectTypeInfo.h>
#include <RE/ConsoleLog.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>

using namespace nlohmann;

namespace {

struct State
{
  std::set<std::string> dumpedClasses;
};

std::string RawTypeToString(RE::BSScript::TypeInfo::RawType raw)
{
  switch (raw) {
    case RE::BSScript::TypeInfo::RawType::kNone:
      return "None";
    case RE::BSScript::TypeInfo::RawType::kObject:
      return "Object";
    case RE::BSScript::TypeInfo::RawType::kString:
      return "String";
    case RE::BSScript::TypeInfo::RawType::kInt:
      return "Int";
    case RE::BSScript::TypeInfo::RawType::kFloat:
      return "Float";
    case RE::BSScript::TypeInfo::RawType::kBool:
      return "Bool";
    case RE::BSScript::TypeInfo::RawType::kNoneArray:
      return "NoneArray";
    case RE::BSScript::TypeInfo::RawType::kObjectArray:
      return "ObjectArray";
    case RE::BSScript::TypeInfo::RawType::kStringArray:
      return "StringArray";
    case RE::BSScript::TypeInfo::RawType::kIntArray:
      return "IntArray";
    case RE::BSScript::TypeInfo::RawType::kFloatArray:
      return "FloatArray";
    case RE::BSScript::TypeInfo::RawType::kBoolArray:
      return "BoolArray";
  }
  return "";
}

json TypeInfoToJson(RE::BSScript::TypeInfo& t)
{
  auto res = json::object();
  auto rawType = t.GetUnmangledRawType();
  res["rawType"] = RawTypeToString(rawType);
  if (rawType == RE::BSScript::TypeInfo::RawType::kObject)
    res["objectTypeName"] = t.GetTypeInfo()->GetName();
  return res;
}

json FunctionToJson(const char* typeName, RE::BSScript::IFunction* f,
                    const std::shared_ptr<PexScript>& pex)
{
  auto res = json::object();
  res["name"] = f->GetName();
  res["returnType"] = TypeInfoToJson(f->GetReturnType());
  res["arguments"] = json::array();
  for (UInt32 i = 0; i < f->GetParamCount(); ++i) {
    RE::BSFixedString name;
    RE::BSScript::TypeInfo type;
    f->GetParam(i, name, type);
    res["arguments"].push_back(
      json{ { "name", name.data() }, { "type", TypeInfoToJson(type) } });

    if (pex) {
      auto it = std::find_if(pex->objectTable.m_data.begin(),
                             pex->objectTable.m_data.end(),
                             [typeName](const ObjectTable::Object& obj) {
                               return !stricmp(obj.NameIndex.data(), typeName);
                             });
      if (it == pex->objectTable.m_data.end())
        throw std::runtime_error("Unable to find object in pex");

      auto stateIt =
        std::find_if(it->states.begin(), it->states.end(),
                     [&](const ObjectTable::Object::StateInfo& state) {
                       return state.name.empty();
                     });
      if (stateIt == it->states.end())
        throw std::runtime_error("Unable to find state in pex object");

      auto funcIt = std::find_if(
        stateIt->functions.begin(), stateIt->functions.end(),
        [&](const ObjectTable::Object::StateInfo::StateFunction& pexF) {
          return !stricmp(pexF.name.data(), f->GetName().data());
        });
      if (funcIt == stateIt->functions.end())
        throw std::runtime_error("Unable to find " +
                                 std::string(f->GetName().data()) + " in pex");

      auto n =
        std::min(funcIt->function.params.size(), res["arguments"].size());
      for (size_t i = 0; i < n; ++i)
        res["arguments"].at(i)["name"] = funcIt->function.params[i].name;
    }
  }

  std::stringstream ss;
  auto funcInfo = GetNativeFunctionAddr::Run(*f);
  auto baseAddr = REL::Module::BaseAddr();
  auto offset = (size_t)funcInfo.fn - baseAddr;
  ss << offset;
  res["offset"] = ss.str();

  res["useLongSignature"] = funcInfo.useLongSignature;
  res["isLatent"] = funcInfo.isLatent;
  return res;
}

void DumpType(State& state, RE::BSScript::ObjectTypeInfo* type, json& out)
{
  std::shared_ptr<PexScript> pex;
  auto p = std::filesystem::path("Data\\Scripts") /
    (type->GetName() + std::string(".pex"));
  if (std::filesystem::exists(p))
    pex = Reader({ p.string() }).GetSourceStructures().at(0);

  json jType = json{ { "globalFunctions", json::array() },
                     { "memberFunctions", json::array() } };

  std::string name = type->GetName();

  if (state.dumpedClasses.count(name))
    return;

  const char* parentName = "PapyrusObjectBase";
  if (auto parent = type->GetParent()) {
    parentName = parent->GetName();
    DumpType(state, parent, out);
  }

  std::map<std::string, RE::BSScript::IFunction*> memberFuncs;
  for (int i = 0; i < type->GetNumMemberFuncs(); ++i) {
    if (auto f = type->GetMemberFuncIter()[i].func; f && f->GetIsNative())
      memberFuncs[f->GetName().data()] = f.get();
  }

  std::map<std::string, RE::BSScript::IFunction*> globalFuncs;
  for (int i = 0; i < type->GetNumGlobalFuncs(); ++i) {
    if (auto f = type->GetGlobalFuncIter()[i].func; f && f->GetIsNative())
      globalFuncs[f->GetName().data()] = f.get();
  }

  for (auto& [name, f] : memberFuncs)
    jType["memberFunctions"].push_back(
      FunctionToJson(type->GetName(), f, pex));

  for (auto& [name, f] : globalFuncs)
    jType["globalFunctions"].push_back(
      FunctionToJson(type->GetName(), f, pex));

  state.dumpedClasses.insert(name);

  if (auto parent = type->GetParent()) {
    jType["parent"] = parent->GetName();
  }

  if (!globalFuncs.empty() || !memberFuncs.empty())
    out["types"][name] = jType;
}
}

void DumpFunctions::Run()
{
  try {
    json out = json{ { "types", json::object() } };

    auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
    if (!vm)
      throw NullPointerException("vm");

    State state;

    std::map<std::string, RE::BSScript::ObjectTypeInfo*> types;
    for (auto& [name, type] : vm->objectTypeMap)
      types[name.data()] = type.get();

    for (auto& [name, type] : types) {
      DumpType(state, type, out);
    }

    std::filesystem::path dir = "Data\\Platform\\Output";
    std::filesystem::create_directories(dir);
    auto p = dir / "FunctionsDump.txt";
    std::ofstream(p) << out.dump(2);

    if (auto c = RE::ConsoleLog::GetSingleton()) {
      std::string s = p.string();
      c->Print("Dumped %d Papyrus types to %s", out["types"].size(), s.data());
    }

  } catch (std::exception& e) {
    if (auto c = RE::ConsoleLog::GetSingleton())
      c->Print("DumpFunctions::Run: %s", e.what());
  }
}