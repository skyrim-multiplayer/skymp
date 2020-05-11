#pragma once
#include <RE/BSFixedString.h>
#include <memory>
#include <string>
#include <unordered_map>

class StringHolder
{
public:
  const RE::BSFixedString& operator[](const std::string& str)
  {
    auto& entry = entries[str];
    if (!entry)
      entry.reset(new Entry(str));
    return entry->GetFixedString();
  }

  static StringHolder& ThreadSingleton()
  {
    thread_local StringHolder stringHolder;
    return stringHolder;
  }

private:
  StringHolder() = default;

  class Entry
  {
  public:
    Entry(const std::string& str)
    {
      holder.reset(new std::string(str));
      fs.reset(new RE::BSFixedString(str.data()));
    }

    ~Entry() { fs.reset(); }

    const RE::BSFixedString& GetFixedString() const noexcept { return *fs; }

  private:
    std::unique_ptr<RE::BSFixedString> fs;
    std::unique_ptr<std::string> holder;
  };
  std::unordered_map<std::string, std::unique_ptr<Entry>> entries;
};