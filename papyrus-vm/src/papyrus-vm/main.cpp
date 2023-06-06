#include "papyrus-vm/Reader.h"
#include "papyrus-vm/VirtualMachine.h"

#include <iostream>

namespace {
void RegisterNatives(std::shared_ptr<VirtualMachine> vm)
{
  vm->RegisterFunction("", "Print", FunctionType::GlobalFunction,
                       [=](VarValue self, const std::vector<VarValue> args) {
                         if (args.size() >= 1) {
                           std::string showString =
                             static_cast<const char*>(args[0]);
                           std::cout << std::endl
                                     << "[!] Papyrus says: " << showString
                                     << std::endl
                                     << std::endl;
                         }
                         return VarValue::None();
                       });
}
}

int main(int argc, char** argv)
{
  if (argc == 2 && std::string(argv[1]) == "--help") {
    std::cout << "Usage: papyrus-vm <script_path> <class_name> "
                 "<function_name> [args...]"
              << std::endl;
    return 0;
  } else if (argc < 4) {
    std::cout << "Usage: papyrus-vm <script_path> <class_name> "
                 "<function_name> [args...]"
              << std::endl;
    return 1;
  } else {
    try {

      std::string script = argv[1];
      std::string className = argv[2];
      std::string functionName = argv[3];

      Reader reader({ script });

      std::vector<std::shared_ptr<PexScript>> vector =
        reader.GetSourceStructures();

      auto vm = std::make_shared<VirtualMachine>(vector);

      RegisterNatives(vm);

      auto args = std::vector<VarValue>{ VarValue::None() };
      vm->CallStatic(className, functionName, args);
    } catch (std::exception& e) {
      std::cout << "Error: " << e.what() << std::endl;
      return 1;
    }
  }
  return 0;
}
