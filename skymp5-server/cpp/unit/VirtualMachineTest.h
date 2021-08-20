#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "Reader.h"
#include "ScriptVariablesHolder.h"
#include "VirtualMachine.h"
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <iterator>
#include <list>
#include <stdexcept>

namespace {
std::shared_ptr<VirtualMachine> CreateVirtualMachine();

class TestObject : public IGameObject
{
public:
  std::string myId = "0x006AFF2E";
  const char* GetStringID() override;
};

class MyScriptVariablesHolder : public ScriptVariablesHolder
{
public:
  MyScriptVariablesHolder(const char* scriptName);

  VarValue* GetVariableByName(const char* name, const PexScript& pex) override;

  std::shared_ptr<IGameObject> testObject;
  VarValue var;
};

}


