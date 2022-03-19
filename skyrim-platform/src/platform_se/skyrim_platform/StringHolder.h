#pragma once

class StringHolder
{
public:
  const FixedString& operator[](const std::string& str)
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
      fs.reset(new FixedString(str.data()));
    }

    ~Entry() { fs.reset(); }

    const FixedString& GetFixedString() const noexcept { return *fs; }

  private:
    std::unique_ptr<FixedString> fs;
    std::unique_ptr<std::string> holder;
  };
  std::unordered_map<std::string, std::unique_ptr<Entry>> entries;
};
