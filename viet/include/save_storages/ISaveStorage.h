#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace Viet {

template <typename T, typename FormDescType>
class ISaveStorage
{
public:
  using IterateSyncCallback = std::function<void(const T&)>;
  using UpsertCallback = std::function<void()>;

  virtual ~ISaveStorage() = default;

  virtual void IterateSync(const IterateSyncCallback& cb) = 0;
  virtual void Upsert(std::vector<std::optional<T>>&& changeForms,
                      const UpsertCallback& cb) = 0;
  virtual uint32_t GetNumFinishedUpserts() const = 0;
  virtual void Tick() = 0;
  virtual bool GetRecycledChangeFormsBuffer(
    std::vector<std::optional<T>>& changeForms) = 0;

  virtual const std::string& GetName() const = 0;
};

namespace ISaveStorageUtils {
template <typename T, typename FormDescType>
inline uint32_t CountSync(ISaveStorage<T, FormDescType>& storage)
{
  uint32_t n = 0;
  storage.IterateSync([&](auto) { ++n; });
  return n;
}

template <typename T, typename FormDescType>
inline std::optional<T> FindSync(ISaveStorage<T, FormDescType>& storage,
                                 const FormDescType& formDesc)
{
  std::optional<T> res;
  storage.IterateSync([&](const T& changeForm) {
    if (changeForm.formDesc == formDesc)
      res = changeForm;
  });
  return res;
}

template <typename T, typename FormDescType>
inline std::map<FormDescType, T> FindAllSync(
  ISaveStorage<T, FormDescType>& storage)
{
  std::map<FormDescType, T> res;
  storage.IterateSync(
    [&](const T& changeForm) { res[changeForm.formDesc] = changeForm; });
  return res;
}
}

}
