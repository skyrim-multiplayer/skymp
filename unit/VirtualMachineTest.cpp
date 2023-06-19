#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "ScriptVariablesHolder.h"
#include "papyrus-vm/Reader.h"
#include "papyrus-vm/VirtualMachine.h"
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <iterator>
#include <list>
#include <stdexcept>

namespace fs = std::filesystem;

namespace {
std::shared_ptr<VirtualMachine> CreateVirtualMachine()
{
  std::vector<std::string> allPath;
  std::vector<fs::path> pexFiles;

  if (!std::filesystem::exists(BUILT_PEX_DIR)) {
    throw std::runtime_error(
      "It seems this machine didn't compile .psc files");
  }

  fs::directory_iterator begin(BUILT_PEX_DIR);
  fs::directory_iterator end;

  std::copy_if(
    begin, end, std::back_inserter(pexFiles), [](const fs::path& path) {
      return fs::is_regular_file(path) && (path.extension() == ".pex");
    });

  for (auto file : pexFiles) {
    allPath.push_back(fs::absolute(file).generic_string());
  }

  REQUIRE(allPath.size() != 0);

  Reader reader(allPath);

  std::vector<std::shared_ptr<PexScript>> vector =
    reader.GetSourceStructures();

  auto vm = std::make_shared<VirtualMachine>(vector);

  std::shared_ptr<int> assertId(new int(1));
  vm->RegisterFunction("", "Print", FunctionType::GlobalFunction,
                       [=](VarValue self, const std::vector<VarValue> args) {
                         if (args.size() >= 1) {
                           std::string showString = (const char*)args[0];
                           std::cout << std::endl
                                     << "[!] Papyrus says: " << showString
                                     << std::endl
                                     << std::endl;
                           (*assertId) = 1;
                         }
                         return VarValue::None();
                       });

  vm->RegisterFunction("", "Assert", FunctionType::GlobalFunction,
                       [=](VarValue self, std::vector<VarValue> args) {
                         if (args.size() >= 1) {
                           bool success = (bool)args[0];
                           std::string message = "\t Assertion " +
                             std::string(success ? "succeed" : "failed") +
                             " (" + std::to_string(*assertId) + ")";
                           (*assertId)++;
                           if (!success) {
                             throw std::runtime_error(message);
                           }
                           std::cout << message << std::endl;
                         }
                         return VarValue::None();
                       });
  vm->RegisterFunction("OpcodesTest", "TestFunction",
                       FunctionType::GlobalFunction,
                       [=](VarValue self, std::vector<VarValue> args) {
                         return VarValue(42); // random integer
                       });

  return vm;
}

class TestObject : public IGameObject
{
public:
  std::string myId = "0x006AFF2E";

  const char* GetStringID() override { return myId.c_str(); };
};

class MyScriptVariablesHolder : public ScriptVariablesHolder
{
public:
  MyScriptVariablesHolder(const char* scriptName)
    : ScriptVariablesHolder(scriptName, {}, {}, nullptr, nullptr, nullptr)
  {
    testObject.reset(new TestObject);
    var = VarValue(testObject.get());
  }

  VarValue* GetVariableByName(const char* name, const PexScript& pex) override
  {
    auto res = ScriptVariablesHolder::GetVariableByName(name, pex);
    if (name == std::string("::OpcodeRef_var")) {
      return &var;
    }
    return res;
  }

  std::shared_ptr<IGameObject> testObject;
  VarValue var;
};

}

TEST_CASE("Real pex parsing and execution", "[VirtualMachine]")
{
  auto vm = CreateVirtualMachine();
  if (!vm)
    return;

  std::vector<VirtualMachine::ScriptInfo> scripts;
  scripts.push_back({ "AAATestObject",
                      std::shared_ptr<ScriptVariablesHolder>(
                        new MyScriptVariablesHolder("AAATestObject")) });
  auto holder = std::shared_ptr<MyScriptVariablesHolder>(
    new MyScriptVariablesHolder("OpcodesTest"));
  scripts.push_back({ "OpcodesTest", holder });
  vm->AddObject(holder->testObject, scripts);

  std::vector<VarValue> functionArgs;
  vm->SendEvent(holder->testObject, "Main", functionArgs);

  //
  // Simple Latent test
  //

  auto latentHolder = std::make_shared<MyScriptVariablesHolder>("LatentTest");
  vm->AddObject(latentHolder->testObject, { { "LatentTest", latentHolder } });

  int nonLatentCalls = 0;

  vm->RegisterFunction("LatentTest", "NonLatentFunc",
                       FunctionType::GlobalFunction,
                       [&](VarValue self, std::vector<VarValue> args) {
                         nonLatentCalls++;
                         return VarValue::None();
                       });

  Viet::Promise<VarValue> promise;

  vm->RegisterFunction("LatentTest", "LatentFunc",
                       FunctionType::GlobalFunction,
                       [promise](VarValue self, std::vector<VarValue> args) {
                         return VarValue(promise);
                       });

  std::vector<VarValue> argsMain;
  vm->CallStatic("LatentTest", "Main", argsMain);

  REQUIRE(nonLatentCalls == 1);

  promise.Resolve(VarValue::None());

  REQUIRE(nonLatentCalls == 2);

  //
  // Return Value Latent test
  //

  auto result = VarValue::None();

  Viet::Promise<VarValue> pr;
  std::vector<VarValue> argsLatentAdd;

  vm->RegisterFunction("LatentTest", "LatentAdd", FunctionType::GlobalFunction,
                       [&](VarValue self, std::vector<VarValue> args) {
                         argsLatentAdd = args;
                         return VarValue(pr);
                       });

  std::vector<VarValue> argsMain2;
  vm->CallStatic("LatentTest", "Main2", argsMain2).Then([&](VarValue v) {
    result = v;
  });

  REQUIRE(result == VarValue::None());
  pr.Resolve(argsLatentAdd[0] + argsLatentAdd[1]);
  REQUIRE(result == VarValue(5));
}

TEST_CASE("Test nested latent calls", "[VirtualMachine]")
{
  auto vm = CreateVirtualMachine();
  if (!vm)
    return;

  struct DoubleTask
  {
    int arg = 0;
    Viet::Promise<VarValue> pr;
  };
  std::list<DoubleTask> t;

  vm->RegisterFunction(
    "LatentTest", "LatentDouble", FunctionType::GlobalFunction,
    [&](VarValue self, const std::vector<VarValue>& args) {
      Viet::Promise<VarValue> pr;
      t.push_back({ static_cast<int>(args[0].CastToInt()), pr });
      return VarValue(pr);
    });

  VarValue result;
  std::vector<VarValue> args;
  vm->CallStatic("LatentTest", "Main3", args).Then([&](VarValue v) {
    result = v;
  });

  REQUIRE(result == VarValue::None());

  while (t.size() > 0) {
    t.front().pr.Resolve(VarValue(t.front().arg * 2));
    t.pop_front();
  }

  REQUIRE(result == VarValue(6));
}
