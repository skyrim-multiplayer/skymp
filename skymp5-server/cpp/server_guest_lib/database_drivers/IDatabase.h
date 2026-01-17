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

class IDatabase
{
public:
  using IterateCallback = std::function<void(const MpChangeForm&)>;

  virtual ~IDatabase() = default;

  // Returns numbers of change forms inserted or updated successfully (Suitable
  // for logging). In practice, it should be equal to `changeForms.size()` when
  // saving succeed.
  size_t Upsert(std::vector<std::optional<MpChangeForm>>&& changeForms);

  virtual void Iterate(const IterateCallback& iterateCallback) = 0;

  bool GetRecycledChangeFormsBuffer(
    std::vector<std::optional<MpChangeForm>>& changeForms);

protected:
  virtual std::vector<std::optional<MpChangeForm>>&& UpsertImpl(
    std::vector<std::optional<MpChangeForm>>&& changeForms,
    size_t& outNumUpserted) = 0;

  std::vector<std::optional<MpChangeForm>> recycledChangeFormsBuffer;
};
