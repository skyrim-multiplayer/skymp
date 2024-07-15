#include "TestUtils.hpp"

#include "MpChangeForms.h"
#include "database_drivers/FileDatabase.h"
#include "database_drivers/ZipDatabase.h"
#include "save_storages/AsyncSaveStorage.h"
#include <filesystem>

std::shared_ptr<ISaveStorage> MakeSaveStorageFile()
{
  auto directory = "unit/data";

  if (std::filesystem::exists(directory)) {
    std::filesystem::remove_all(directory);
  }

  return std::make_shared<AsyncSaveStorage>(
    std::make_shared<FileDatabase>(directory, spdlog::default_logger()),
    spdlog::default_logger(), "file");
}

std::shared_ptr<ISaveStorage> MakeSaveStorageZip()
{
  auto archivePath = "world.zip";

  if (std::filesystem::exists(archivePath)) {
    std::filesystem::remove(archivePath);
  }

  return std::make_shared<AsyncSaveStorage>(
    std::make_shared<ZipDatabase>(archivePath, spdlog::default_logger()),
    spdlog::default_logger(), "zip");
}

std::vector<std::shared_ptr<ISaveStorage>> MakeSaveStorages()
{
  return { MakeSaveStorageFile(), MakeSaveStorageZip() };
}

MpChangeForm CreateChangeForm(const char* descStr)
{
  MpChangeForm res;
  res.formDesc = FormDesc::FromString(descStr);
  return res;
}

void UpsertSync(ISaveStorage& st,
                std::vector<std::optional<MpChangeForm>> changeForms)
{
  bool finished = false;
  st.Upsert(std::move(changeForms), [&] { finished = true; });

  int i = 0;
  while (!finished) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    st.Tick();
    ++i;
    if (i > 2000)
      throw std::runtime_error("Timeout exceeded");
  }
}

void WaitForNextUpsert(ISaveStorage& st, WorldState& wst)
{
  uint32_t n = st.GetNumFinishedUpserts();

  int i = 0;
  while (n == st.GetNumFinishedUpserts()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    wst.Tick(); // should include st.Tick()

    ++i;
    if (i > 2000)
      throw std::runtime_error("Timeout exceeded");
  }
}

TEST_CASE("ChangeForm with spaces is saved correctly", "[save]")
{
  auto storagesToTest = MakeSaveStorages();

  for (const auto& st : storagesToTest) {
    SECTION("Testing with " + st->GetName())
    {
      MpChangeForm f1;
      f1.formDesc = { 1, "" };

      Appearance appearance;
      appearance.name = "La La La";
      appearance.weight = 0.1;

      f1.appearanceDump = appearance.ToJson();

      UpsertSync(*st, { f1 });

      auto res = ISaveStorageUtils::FindAllSync(*st);
      REQUIRE(res.size() == 1);
      REQUIRE(res[{ 1, "" }].appearanceDump == appearance.ToJson());
    }
  }
}

TEST_CASE("ChangeForm is saved correctly", "[save]")
{
  auto storagesToTest = MakeSaveStorages();

  for (const auto& st : storagesToTest) {
    SECTION("Testing with " + st->GetName())
    {
      MpChangeForm f1, f2;
      f1.formDesc = { 1, "" };
      f1.position = { 1, 2, 3 };
      f1.appearanceDump = "{}";
      f1.inv.AddItem(0xf, 1000);
      f1.equipmentDump = "[]";
      f1.actorValues.healthPercentage = 0.25f;
      f1.actorValues.magickaPercentage = 0.3f;
      f1.actorValues.staminaPercentage = 1.0f;
      f2.formDesc = { 2, "" };
      f2.position = { 2, 4, 6 };
      f2.isDisabled = true;
      f2.profileId = 10;
      f2.actorValues.healthPercentage = 0;
      f2.actorValues.magickaPercentage = 0;
      f2.actorValues.staminaPercentage = 0;
      UpsertSync(*st, { f1, f2 });
      Faction faction = Faction();
      faction.formDesc = FormDesc(13, "Skyrim.esm");
      faction.rank = 10;
      f2.factions = std::vector<Faction>();
      f2.factions.value().push_back(faction);
      UpsertSync(*st, { f1, f2 });

      auto res = ISaveStorageUtils::FindAllSync(*st);
      REQUIRE(res.size() == 2);
      REQUIRE(res[{ 1, "" }].position == NiPoint3(1, 2, 3));
      REQUIRE(res[{ 1, "" }].appearanceDump == "{}");
      REQUIRE(res[{ 1, "" }].equipmentDump == "[]");
      REQUIRE(res[{ 1, "" }].inv == Inventory().AddItem(0xf, 1000));
      REQUIRE(res[{ 1, "" }].isDisabled == false);
      REQUIRE(res[{ 1, "" }].profileId == -1);
      REQUIRE(res[{ 1, "" }].actorValues.healthPercentage == 0.25f);
      REQUIRE(res[{ 1, "" }].actorValues.magickaPercentage == 0.3f);
      REQUIRE(res[{ 1, "" }].actorValues.staminaPercentage == 1.0f);
      REQUIRE(res[{ 2, "" }].position == NiPoint3(2, 4, 6));
      REQUIRE(res[{ 2, "" }].isDisabled == true);
      REQUIRE(res[{ 2, "" }].profileId == 10);
      REQUIRE(res[{ 2, "" }].actorValues.healthPercentage == 0);
      REQUIRE(res[{ 2, "" }].actorValues.magickaPercentage == 0);
      REQUIRE(res[{ 2, "" }].actorValues.staminaPercentage == 0);
      REQUIRE(res[{ 2, "" }].factions.has_value());
      REQUIRE(res[{ 2, "" }].factions.value().size() == 1);
      REQUIRE(res[{ 2, "" }].factions.value()[0].formDesc.shortFormId == 13);
      REQUIRE(res[{ 2, "" }].factions.value()[0].formDesc.file ==
              "Skyrim.esm");
      REQUIRE(res[{ 2, "" }].factions.value()[0].rank == 10);
    }
  }
}

TEST_CASE("Upsert affects the number of change forms in the database in the "
          "correct way",
          "[save]")
{
  auto storagesToTest = MakeSaveStorages();

  for (const auto& st : storagesToTest) {
    SECTION("Testing with " + st->GetName())
    {
      REQUIRE(ISaveStorageUtils::CountSync(*st) == 0);

      UpsertSync(*st,
                 { CreateChangeForm("0"), CreateChangeForm("1"),
                   CreateChangeForm("2") });

      REQUIRE(ISaveStorageUtils::CountSync(*st) == 3);

      UpsertSync(*st,
                 { CreateChangeForm("0"), CreateChangeForm("1"),
                   CreateChangeForm("2"), CreateChangeForm("3") });

      REQUIRE(ISaveStorageUtils::CountSync(*st) == 4);
    }
  }
}

TEST_CASE("AttachSaveStorage forces loading", "[save]")
{
  auto storagesToTest = MakeSaveStorages();

  for (const auto& st : storagesToTest) {
    SECTION("Testing with " + st->GetName())
    {
      PartOne p;
      p.worldState.espmFiles = { "AaAaAa.esm" };
      p.worldState.AddForm(
        std::unique_ptr<MpObjectReference>(new MpObjectReference(
          LocationalData(), FormCallbacks::DoNothing(), 0xaaaa, "STAT")),
        0xee);

      auto& refr = p.worldState.GetFormAt<MpObjectReference>(0xee);
      REQUIRE(refr.GetPos() == NiPoint3(0, 0, 0));

      auto f = CreateChangeForm("ee:AaAaAa.esm");
      f.position = { 1, 1, 1 };
      f.baseDesc = FormDesc::FromString("aaaa:AaAaAa.esm");
      UpsertSync(*st, { f });
      p.AttachSaveStorage(st);

      REQUIRE(refr.GetPos() == NiPoint3(1, 1, 1));
    }
  }
}

TEST_CASE("Changes are transferred to SaveStorage", "[save]")
{
  auto storagesToTest = MakeSaveStorages();

  for (const auto& st : storagesToTest) {
    SECTION("Testing with " + st->GetName())
    {
      PartOne p;
      p.AttachSaveStorage(st);

      REQUIRE(ISaveStorageUtils::CountSync(*st) == 0);
      p.CreateActor(0xffaaaeee, { 1, 1, 1 }, 1, 0x3c);

      WaitForNextUpsert(*st, p.worldState);
      REQUIRE(ISaveStorageUtils::CountSync(*st) == 1);
    }
  }
}
