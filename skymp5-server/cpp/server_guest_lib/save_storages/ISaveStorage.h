#pragma once
#include "MpChangeForms.h"
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

class ISaveStorage
{
public:
  using IterateSyncCallback = std::function<void(const MpChangeForm&)>;
  using UpsertCallback = std::function<void()>;

  virtual void IterateSync(const IterateSyncCallback& cb) = 0;
  virtual void Upsert(std::vector<std::optional<MpChangeForm>>&& changeForms,
                      const UpsertCallback& cb) = 0;
  virtual uint32_t GetNumFinishedUpserts() const = 0;
  virtual void Tick() = 0;
  virtual bool GetRecycledChangeFormsBuffer(
    std::vector<std::optional<MpChangeForm>>& changeForms) = 0;

  virtual const std::string& GetName() const = 0;
};

namespace ISaveStorageUtils {
inline uint32_t CountSync(ISaveStorage& storage)
{
  uint32_t n = 0;
  storage.IterateSync([&](auto) { ++n; });
  return n;
}

inline std::optional<MpChangeForm> FindSync(ISaveStorage& storage,
                                            const FormDesc& formDesc)
{
  std::optional<MpChangeForm> res;
  storage.IterateSync([&](const MpChangeForm& changeForm) {
    if (changeForm.formDesc == formDesc)
      res = changeForm;
  });
  return res;
}

inline std::map<FormDesc, MpChangeForm> FindAllSync(ISaveStorage& storage)
{
  std::map<FormDesc, MpChangeForm> res;
  storage.IterateSync([&](const MpChangeForm& changeForm) {
    res[changeForm.formDesc] = changeForm;
  });
  return res;
}
}
