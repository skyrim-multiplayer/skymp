#pragma once
#include <string>
#include <vector>
#include <memory>

class StringTable
{
public:
  // Do NOT mutate storage after pex loading. reallocation would make string
  // VarValues invalid
  void SetStorage(std::vector<std::string> newStorage)
  {
    storage = std::move(newStorage);
  }

  std::vector<std::shared_ptr<std::string>> instanceStringTable;

  const std::vector<std::string>& GetStorage() const { return storage; }

private:
  std::vector<std::string> storage;
};
