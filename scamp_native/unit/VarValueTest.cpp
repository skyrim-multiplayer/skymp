#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "OpcodesImplementation.h"
#include "Structures.h"
#include <cstdint>
#include <stdexcept>

TEST_CASE("test bool operators (<, <=, >, >=)", "[VarValue]")
{
  VarValue bool1(false);
  VarValue bool2(true);
  REQUIRE(bool(bool1 < bool2));
  REQUIRE(bool(bool1 <= bool2));
  REQUIRE(bool(bool2 > bool1));
  REQUIRE(bool(bool2 >= bool1));
}

TEST_CASE("operator= for owning objects", "[VarValue]")
{
  class MyObject : public IGameObject
  {
  };

  VarValue var;
  var = VarValue(std::make_shared<MyObject>());

  REQUIRE(dynamic_cast<MyObject*>(static_cast<IGameObject*>(var)));
}

TEST_CASE("operator== for objects", "[VarValue]")
{
  class MyObject : public IGameObject
  {
  public:
    MyObject(int i) { this->i = i; }

    bool EqualsByValue(const IGameObject& rhs) const override
    {
      if (auto rhsMy = dynamic_cast<const MyObject*>(&rhs))
        return i == rhsMy->i;
      return false;
    }

  private:
    int i;
  };

  REQUIRE(VarValue(std::make_shared<MyObject>(1)) ==
          VarValue(std::make_shared<MyObject>(1)));
  REQUIRE(VarValue(std::make_shared<MyObject>(1)) !=
          VarValue(std::make_shared<MyObject>(2)));
}

TEST_CASE("VarValue Identifier", "[VarValue]")
{
  VarValue IdentifierConstructor = VarValue(uint8_t(1));
  VarValue Identifier = VarValue(uint8_t(1), "kType_Identifier");
  std::string err = "";

  try {
    auto t = !Identifier;
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";

  try {
    auto t = Identifier.CastToInt();
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";

  try {
    auto t = Identifier.CastToFloat();
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";

  try {
    auto t = Identifier.CastToBool();
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";
}

TEST_CASE("VarValue with nonexistent Type", "[VarValue]")
{
  std::string err = "";

  try {
    VarValue nonexistent = VarValue(uint8_t(300));
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";
}

TEST_CASE("wrong types", "[VarValue]")
{
  VarValue str1("string1");
  VarValue str2("string2");

  std::string err = "";

  // operators test

  try {
    bool t = str1 > str2;
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";

  try {
    bool t = str1 >= str2;
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";

  try {
    bool t = str1 < str2;
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";

  try {
    bool t = str1 <= str2;
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";

  try {
    auto t = str1 + str2;
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";

  try {
    auto t = str1 - str2;
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";

  try {
    auto t = str1 * str2;
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";

  try {
    auto t = str1 / str2;
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";

  try {
    auto t = str1 % str2;
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";

  // Cast Functions

  try {
    auto t = str1.CastToInt();
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";

  try {
    auto t = str1.CastToFloat();
  } catch (std::exception& e) {
    err = e.what();
  }
  REQUIRE(err != "");
  err = "";
}

TEST_CASE("strcat implicit casts", "[VarValue]")
{
  StringTable stringTable;
  auto res = OpcodesImplementation::StrCat(VarValue::None(), VarValue("_abc"),
                                           stringTable);
  REQUIRE(res == VarValue("None_abc"));
}