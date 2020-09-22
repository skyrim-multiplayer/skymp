#include "TestUtils.hpp"

#include "MpChangeForms.h"
#include "SqliteSaveStorage.h"
#include <filesystem>

std::shared_ptr<ISaveStorage> MakeSaveStorage()
{
  auto fileName = "unit.sqlite";
  if (std::filesystem::exists(fileName))
    std::filesystem::remove(fileName);
  return std::make_shared<SqliteSaveStorage>(fileName);
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
    if (i > 2000)
      throw std::runtime_error("Timeout exceeded");
  }
}

TEST_CASE("ChangeForm is saved correctly", "[save]")
{
  auto st = MakeSaveStorage();

  MpChangeForm f1, f2;
  f1.formDesc = { 1, "" };
  f1.position = { 1, 2, 3 };
  f2.formDesc = { 2, "" };
  f2.position = { 2, 4, 6 };
  UpsertSync(*st, { f1, f2 });

  auto res = ISaveStorageUtils::FindAllSync(*st);
  REQUIRE(res.size() == 2);
  REQUIRE(res[{ 1, "" }].position == NiPoint3(1, 2, 3));
  REQUIRE(res[{ 2, "" }].position == NiPoint3(2, 4, 6));
}

TEST_CASE("Upsert affects the number of change forms in the database in the "
          "correct way",
          "[save]")
{
  auto st = MakeSaveStorage();

  REQUIRE(ISaveStorageUtils::CountSync(*st) == 0);

  UpsertSync(
    *st,
    { CreateChangeForm("0"), CreateChangeForm("1"), CreateChangeForm("2") });

  REQUIRE(ISaveStorageUtils::CountSync(*st) == 3);

  UpsertSync(*st,
             { CreateChangeForm("0"), CreateChangeForm("1"),
               CreateChangeForm("2"), CreateChangeForm("3") });

  REQUIRE(ISaveStorageUtils::CountSync(*st) == 4);
}