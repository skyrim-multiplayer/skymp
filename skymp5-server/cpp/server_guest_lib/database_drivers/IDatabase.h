#pragma once
#include "MpChangeForms.h"
#include <functional>
#include <optional>
#include <stdexcept>
#include <vector>

class UpsertFailedException : public std::runtime_error
{
public:
  UpsertFailedException(
    std::vector<std::optional<MpChangeForm>>&& affectedForms_,
    std::string what)
    : runtime_error(what)
    , affectedForms(affectedForms_)
  {
  }

  const std::vector<std::optional<MpChangeForm>>& GetAffectedForms()
    const noexcept
  {
    return affectedForms;
  }

private:
  const std::vector<std::optional<MpChangeForm>> affectedForms;
};

struct UpsertResult
{
  size_t numUpserted = 0;
  std::optional<std::string> changeFormsCollectionHash;
};

class IDatabase
{
public:
  using IterateCallback = std::function<void(const MpChangeForm&)>;

  virtual ~IDatabase() = default;

  // Returns numbers of change forms inserted or updated successfully (Suitable
  // for logging). In practice, it should be equal to `changeForms.size()` when
  // saving succeed.
  virtual UpsertResult Upsert(
    std::vector<std::optional<MpChangeForm>>&& changeForms) = 0;

  virtual void Iterate(const IterateCallback& iterateCallback) = 0;
};
