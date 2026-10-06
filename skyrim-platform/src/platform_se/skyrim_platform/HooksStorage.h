#pragma once
#include <map>
#include <mutex>
#include <string>
#include <variant>

// Thread-safe key-value storage shared between Node.js and QuickJS contexts.
// Values are stored as C++ variants; both runtimes get/set through the same
// mutex-protected map.
class HooksStorage
{
public:
  using Value = std::variant<std::monostate, bool, double, std::string>;

  static HooksStorage& GetSingleton();

  void Set(const std::string& key, Value value);
  Value Get(const std::string& key) const;
  bool Has(const std::string& key) const;
  void Erase(const std::string& key);
  void Clear();

  // Snapshot all keys for enumeration (avoids holding the lock during
  // iteration on the JS side)
  std::map<std::string, Value> Snapshot() const;

private:
  mutable std::mutex mtx;
  std::map<std::string, Value> data;
};
