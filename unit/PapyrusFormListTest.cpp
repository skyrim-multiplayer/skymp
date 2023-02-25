#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "EspmGameObject.h"
#include "PapyrusFormList.h"

extern espm::Loader l;

TEST_CASE("GetSize/GetAt", "[Papyrus][FormList][espm]")
{
  auto& br = l.GetBrowser();

  auto record = br.LookupById(0x21e81);

  auto formlist = VarValue(std::make_shared<EspmGameObject>(record));

  REQUIRE(PapyrusFormList().GetSize(formlist, {}) == VarValue(2));

  VarValue element0 = PapyrusFormList().GetAt(formlist, { VarValue(0) });
  REQUIRE(GetRecordPtr(element0).rec->GetId() == 0x3eab9);

  VarValue element1 = PapyrusFormList().GetAt(formlist, { VarValue(1) });
  REQUIRE(GetRecordPtr(element1).rec->GetId() == 0x4e4bb);

  REQUIRE(PapyrusFormList().GetAt(formlist, { VarValue(-1) }) ==
          VarValue::None());
  REQUIRE(PapyrusFormList().GetAt(formlist, { VarValue(2) }) ==
          VarValue::None());
}
