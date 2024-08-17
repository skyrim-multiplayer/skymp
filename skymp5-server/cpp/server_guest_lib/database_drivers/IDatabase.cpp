#include "IDatabase.h"

size_t IDatabase::Upsert(
  std::vector<std::optional<MpChangeForm>>&& changeForms)
{
  size_t numUpserted = 0;

  recycledChangeFormsBuffer = UpsertImpl(std::move(changeForms), numUpserted);

  return numUpserted;
}

bool IDatabase::GetRecycledChangeFormsBuffer(
  std::vector<std::optional<MpChangeForm>>& changeForms)
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
