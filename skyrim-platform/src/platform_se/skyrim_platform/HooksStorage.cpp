#include "HooksStorage.h"

HooksStorage& HooksStorage::GetSingleton()
{
  static HooksStorage instance;
  return instance;
}

void HooksStorage::Set(const std::string& key, Value value)
{
  std::lock_guard lock(mtx);
  data[key] = std::move(value);
}

HooksStorage::Value HooksStorage::Get(const std::string& key) const
{
  std::lock_guard lock(mtx);
  auto it = data.find(key);
  if (it == data.end()) {
    return std::monostate{};
  }
  return it->second;
}

bool HooksStorage::Has(const std::string& key) const
{
  std::lock_guard lock(mtx);
  return data.count(key) > 0;
}

void HooksStorage::Erase(const std::string& key)
{
  std::lock_guard lock(mtx);
  data.erase(key);
}

void HooksStorage::Clear()
{
  std::lock_guard lock(mtx);
  data.clear();
}

std::map<std::string, HooksStorage::Value> HooksStorage::Snapshot() const
{
  std::lock_guard lock(mtx);
  return data;
}
