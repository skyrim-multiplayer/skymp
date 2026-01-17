#pragma once
#include <functional>
#include <optional>
#include <stdexcept>
#include <vector>

template <typename T>
class UpsertFailedException : public std::runtime_error
{
public:
  UpsertFailedException(std::vector<std::optional<T>>&& affectedForms_,
                        std::string what)
    : runtime_error(what)
    , affectedForms(affectedForms_)
  {
  }

  const std::vector<std::optional<T>>& GetAffectedForms() const noexcept
  {
    return affectedForms;
  }

private:
  const std::vector<std::optional<T>> affectedForms;
};

template <typename T>
class IDatabase
{
public:
  using IterateCallback = std::function<void(const T&)>;

  virtual ~IDatabase() = default;

  // Returns numbers of change forms inserted or updated successfully (Suitable
  // for logging). In practice, it should be equal to `changeForms.size()` when
  // saving succeed.
  size_t Upsert(std::vector<std::optional<T>>&& changeForms)
  {
    size_t numUpserted = 0;
    recycledChangeFormsBuffer =
      UpsertImpl(std::move(changeForms), numUpserted);
    return numUpserted;
  }

  virtual void Iterate(const IterateCallback& iterateCallback) = 0;

  bool GetRecycledChangeFormsBuffer(std::vector<std::optional<T>>& changeForms)
  {
    if (recycledChangeFormsBuffer.empty()) {
      return false;
    }

    for (auto& value : recycledChangeFormsBuffer) {
      value = std::nullopt;
    }

    changeForms = std::move(recycledChangeFormsBuffer);
    recycledChangeFormsBuffer.clear();
    return true;
  }

protected:
  virtual std::vector<std::optional<T>>&& UpsertImpl(
    std::vector<std::optional<T>>&& changeForms, size_t& outNumUpserted) = 0;

  std::vector<std::optional<T>> recycledChangeFormsBuffer;
};
