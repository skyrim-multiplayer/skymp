#pragma once
#include "MpChangeForms.h"
#include <functional>
#include <optional>

class IDatabase
{
public:
  using IterateCallback = std::function<void(const MpChangeForm&)>;

  virtual ~IDatabase() = default;

  // Returns numbers of change forms inserted or updated successfully (Suitable
  // for logging). In practice, it should be equal to `changeForms.size()` when
  // saving succeed.
  virtual size_t Upsert(const std::vector<MpChangeForm>& changeForms) = 0;

  virtual std::optional<MpChangeForm> FindOne(const FormDesc &formDesc) = 0;

  virtual void Iterate(const IterateCallback& iterateCallback) = 0;
};
