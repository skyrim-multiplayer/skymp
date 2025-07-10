#pragma once
#include <string>

namespace FunctionsDumpFormat {
    struct Root;
}

class NirnLabCodegen {
public:
  static std::string GenerateCode(const FunctionsDumpFormat::Root& root);
};
