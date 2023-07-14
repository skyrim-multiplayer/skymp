#include "PartOne.h"
#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "PapyrusGame.h"
#include "papyrus-vm/Structures.h"

PartOne& GetPartOne();

TEST_CASE("GetForm", "[Papyrus][Game][espm]")
{
  PartOne& partOne = GetPartOne();
  PapyrusGame game;
  constexpr const uint32_t foodBarrel = 0x20570;
  const auto& refer =
    partOne.worldState.GetFormAt<MpObjectReference>(foodBarrel);
  const IGameObject* pForm =
    partOne.worldState.LookupFormById(refer.GetFormId())->ToGameObject().get();
  const IGameObject* pPapyrusForm =
    static_cast<const IGameObject*>(game.GetFormEx(
      VarValue::None(), { VarValue(static_cast<int32_t>(foodBarrel)) }));
  REQUIRE(pForm == pPapyrusForm);
}

TEST_CASE("GetFormEx", "[Papyrus][Game][espm]")
{
  PartOne& partOne = GetPartOne();
  PapyrusGame game;
  DoConnect(partOne, 0);
  const uint32_t formId =
    partOne.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  partOne.SetUserActor(0, formId);
  const IGameObject* pForm =
    partOne.worldState.LookupFormById(formId)->ToGameObject().get();
  const IGameObject* pPapyrusForm =
    static_cast<const IGameObject*>(game.GetFormEx(
      VarValue::None(), { VarValue(static_cast<int32_t>(formId)) }));
  REQUIRE(pForm == pPapyrusForm);
}
