#include "TestUtils.hpp"
#include <catch2/catch.hpp>

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