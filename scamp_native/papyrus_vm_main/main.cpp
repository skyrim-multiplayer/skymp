#include "Reader.h"
#include "VirtualMachine.h"
#include <cstdint>
#include <ctime>
#include "tests.h"

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING

#include <experimental/filesystem>
#include <iterator>

namespace fs = std::experimental::filesystem;

int main(int argc, char* argv[])
{
  std::cout << "Running unit tests for VM" << std::endl;
  int result = Catch::Session().run(argc, argv);
  if (result != 0) {
    return result;
  }

  std::vector<std::string> allPath;
  std::vector<fs::path> pexFiles;

  const fs::path pathTo = fs::current_path();
  fs::directory_iterator begin("pex");
  fs::directory_iterator end;

  std::copy_if(
    begin, end, std::back_inserter(pexFiles), [](const fs::path& path) {
      return fs::is_regular_file(path) && (path.extension() == ".pex");
    });

  for (auto file : pexFiles) {
    allPath.push_back(fs::absolute(file).generic_string());
  }

  try {

    Reader reader(allPath);

    std::vector<std::shared_ptr<PexScript>> vector =
      reader.GetSourceStructures();

    VirtualMachine vm(vector);

    std::shared_ptr<int> assertId(new int(1));
    vm.RegisterFunction("", "Print", FunctionType::GlobalFunction,
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

    vm.RegisterFunction("", "Assert", FunctionType::GlobalFunction,
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
    vm.RegisterFunction("OpcodesTest", "TestFunction", FunctionType::GlobalFunction,
                        [=](VarValue self, std::vector<VarValue> args) {
                          return VarValue(42); //random integer
                        });

    class TestObject : public IGameObject
    {
      const std::string MY_ID = "0x006AFF2E";

    public:
      const char* GetStringID() override { return MY_ID.c_str(); };
    };

    std::shared_ptr<IGameObject> testObject(new TestObject);

    VarForBuildActivePex vars;

    std::vector<std::pair<std::string, VarValue>> mapArgs;

    mapArgs.push_back(std::make_pair<std::string, VarValue>(
      "OpcodeRef", VarValue(testObject.get())));

    vars["OpcodesTest"] = mapArgs;

    vm.AddObject(testObject, { "AAATestObject", "OpcodesTest" }, vars);

    std::vector<VarValue> functionArgs;
    vm.SendEvent(testObject, "Main", functionArgs);

    return 0;

  } catch (std::exception& e) {
    std::cerr << "\n\n\n[!!!] Unhandled exception\n\t" << e.what() << "\n\n\n"
              << std::endl;
    return 1;
  }
}
