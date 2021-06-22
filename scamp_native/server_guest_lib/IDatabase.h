#pragma once
#include "MpChangeForms.h"
#include <functional>

class IDatabase
{
public:
  using IterateCallback = std::function<void(const MpChangeForm&)>;

  virtual ~IDatabase() = default;
  virtual size_t Upsert(const std::vector<MpChangeForm>& changeForms) = 0;
  virtual void Iterate(const IterateCallback& iterateCallback) = 0;
};