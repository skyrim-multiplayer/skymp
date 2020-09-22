#include "TestUtils.hpp"

#include "MpChangeForms.h"
#include "SqliteSaveStorage.h"

std::shared_ptr<ISaveStorage> MakeSaveStorage()
{
  return std::make_shared<SqliteSaveStorage>("");
}

MpChangeForm CreateChangeForm(const char* descStr)
{
  MpChangeForm res;
  res.formDesc = FormDesc::FromString(descStr);
  return res;
}

void UpsertSync(ISaveStorage& st, std::vector<MpChangeForm> changeForms)
{
  bool finished = false;
  st.Upsert(changeForms, [&] { finished = true; });

  int i = 0;
  while (!finished) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    st.Tick();
    ++i;
    if (i > 1000)
      throw std::runtime_error("Timeout exceeded");
  }
}

TEST_CASE("", "[save]")
{
  auto st = MakeSaveStorage();

  REQUIRE(ISaveStorageUtils::CountSync(*st) == 0);

  UpsertSync(
    *st,
    { CreateChangeForm("0"), CreateChangeForm("1"), CreateChangeForm("2") });

  REQUIRE(ISaveStorageUtils::CountSync(*st) == 3);

  // ISaveStorageUtils::FindSync(*st);

  PartOne partOne;
}