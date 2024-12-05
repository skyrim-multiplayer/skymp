#include "TestUtils.hpp"
#include "archives/SimdJsonInputArchive.h"
#include "papyrus-vm/VarValue.h"
#include <catch2/catch_all.hpp>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/internal/catch_to_string.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <exception>
#include <nlohmann/detail/conversions/to_json.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <simdjson.h>
#include <string>
#include <type_traits>
#include <variant>

#define MAKE_DOM_ELEMENT_VAR_FROM_STRING(varName, s) \
  std::string tmpString_##varName = s; \
  simdjson::dom::parser parser_##varName; \
  const auto varName = parser_##varName.parse(tmpString_##varName).value();

// MAKE_DOM_ELEMENT_VAR_FROM_STRING(kek, "123");

namespace {
// XXX: remove long double?
using JsonTestParam = std::variant<bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double, long double, std::string>;
using Catch::Matchers::ContainsSubstring;

// simdjson::dom::element DomElementFromString(const std::string& s) {
//   std::string jsonStr = s;
//   simdjson::dom::parser sjParser;
//   // return sjParser.parse(jsonStr).value();
//   auto result = sjParser.parse(s);
//   // std::cerr << "XXX " << s << "\n";
//   // std::cerr << "XXX " << simdjson::to_string(result) << "\n";
//   // std::cerr << "XXX " << simdjson::to_string(result) << "\n";
//   return result.value();
// }

template <class Target>
struct CheckHelper {
  simdjson::dom::parser parser; // we have to keep it, otherwise dom element invalidates
  simdjson::dom::element element;
  Target result;

  void Parse(const std::string& s) {
    element = parser.parse(s);
    SimdJsonInputArchive ar(element);
    ar.Serialize(result);
  }
};

}

// also to anon ns?
namespace Catch {
template<>
struct StringMaker<JsonTestParam> {
  static std::string convert(const JsonTestParam & value ) {
    // std::terminate();
    return std::visit([](auto&& arg) {
      // std::cerr << "XXX " << typeid(decltype(arg)).name() << "\n";
      // Catch::to_string(arg);
      return StringMaker<decltype(arg)>{}.convert(arg);
    }, value);
  }
};
}

TEST_CASE("SimdJsonArchive simple",
          "[Archives]")
{
  // auto i = GENERATE();
  // auto test = []<typename T>(T t) {
  //   // nlohmann::to_json()
  //   nlohmann::json j{t};
  //   CAPTURE(nlohmann::to_string(j));
  //   INFO("test");
  // };

  // test(static_cast<int16_t>(123));

  // nlohmann::json j{static_cast<int16_t>(123)};
  // CAPTURE(nlohmann::to_string(j));
  //   INFO("test7");

//   std::variant<int, std::string> kek;
//   kek = 7;
//   CAPTURE(kek);
// //   {
// // Catch ::Capturer capturer6(
// //   "CAPTURE"_catch_sr,
// //   ::Catch ::SourceLineInfo("/home/roma/my/skymp/skymp/unit/ArchiveTest.cpp",
// //                            static_cast<std ::size_t>(30)),
// //   Catch ::ResultWas ::Info, "kek"_catch_sr);
// // capturer6.captureValues(0, kek)
// //   }
//   kek = "kek";
//   CAPTURE(kek);

// The supported types are ondemand::object, ondemand::array, raw_json_string, std::string_view, uint64_t, int64_t, double, and bool

  JsonTestParam param = GENERATE(
    // one param is enough, type will be deduced for the others
    JsonTestParam("test_string"),
    false,
    true,
    // ""
    '7',
    static_cast<int8_t>(42),
    static_cast<uint8_t>(42),
    static_cast<int16_t>(42),
    static_cast<uint16_t>(42),
    static_cast<int32_t>(42),
    static_cast<uint32_t>(42),
    static_cast<uint32_t>(42),
    static_cast<uint64_t>(42),
    static_cast<uint64_t>(42),
    13.37,
    13.37f,
    // 13.37L, // long double; not sure if it should be tested or not. We'd lose precision here anyway
    ""
    // etc
  );
  CAPTURE(param);

  nlohmann::json j;
  auto type = std::visit([&j](const auto& val) {
    j = val;
    return typeid(decltype(val)).name();
  }, param);
  CAPTURE(type);
  auto jsonStr = nlohmann::to_string(j);
  CAPTURE(jsonStr);

  // simdjson::ondemand::parser sjParser;
  simdjson::dom::parser sjParser;
  // auto padded = simdjson::padded_string(std::move(jsonStr));
  // auto sj = sjParser.iterate(padded);
  auto sj = sjParser.parse(jsonStr);
  // static_assert(!sizeof(decltype(sj)));
  
  SimdJsonInputArchive ar(sj.value());
  JsonTestParam outVar;
  std::visit([&ar, &outVar](const auto& val) {
    // val is also optional; pick the same slot
    std::decay_t<decltype(val)> arOut;
    ar.Serialize(arOut);
    outVar = arOut;
  }, param);
  REQUIRE(param.index() == outVar.index());
  REQUIRE(param == outVar);

  // bool isFloat;
  // Catch::Matchers::WithinRel()
  // std::visit([&](const auto& val) {
  //   isFloat = std::is_floating_point_v<decltype(val)>;
  // }, param);
  // if (isFloat) {
  //   REQUIRE_THAT(arg, matcher)
  // } else {
  //   REQUIRE(param == outVar);
  // }

  // FAIL();
}

// TEST_CASE("SimdJsonArchive objects",
//           "[Archives]")
// {
//   struct Case {

//   };
// }

TEST_CASE("SimdJsonArchive array",
          "[Archives]")
{
  MAKE_DOM_ELEMENT_VAR_FROM_STRING(sj, "[1, 2, 3]");
  CAPTURE(simdjson::to_string(sj));
  SimdJsonInputArchive ar(sj);
  CAPTURE(simdjson::to_string(sj));

  INFO("hi!");
  {
    std::array<int, 3> a;
    ar.Serialize(a);
    REQUIRE(a == std::array<int, 3>{1, 2, 3});
  }

  INFO("hi!");
  {
    CheckHelper<std::array<int, 3>> h;
    h.Parse("[1, 2, 3]");
    REQUIRE(h.result == std::array<int, 3>{1, 2, 3});
  }

  INFO("hi!");
  {
    std::array<int, 2> a;
    REQUIRE_THROWS_WITH(ar.Serialize(a), "index 2 out of bounds for output");
  }

  INFO("hi!");
  {
    std::array<int, 4> a;
    REQUIRE_THROWS_WITH(ar.Serialize(a), "index 3 out of bounds for input");
  }

  INFO("hi!");
  {
    std::array<std::string, 3> a;
    REQUIRE_THROWS_WITH(ar.Serialize(a), ContainsSubstring("index 0:") && ContainsSubstring("INCORRECT_TYPE"));
  }

  INFO("hi!");
  {
    MAKE_DOM_ELEMENT_VAR_FROM_STRING(sj2, "[]");
    SimdJsonInputArchive ar2(sj2);
    std::array<int, 3> a;
    REQUIRE_THROWS_WITH(ar2.Serialize(a), "index 0 out of bounds for input");
  }

  INFO("hi!");
  {
    CheckHelper<std::array<int, 3>> h;
    REQUIRE_THROWS_WITH(h.Parse("[]"), "index 0 out of bounds for input");
  }
}

/*
TEST_CASE("SimdJsonArchive vector",
          "[Archives]")
{
  // struct Case {
  //   std::string json;
  //   Matcher
  // };

  {
    const auto sj = DomElementFromString("[1, 2, 3]");
    SimdJsonInputArchive ar(sj);
    std::vector<int> a;
    ar.Serialize(a);
    REQUIRE(a == std::vector<int>{1, 2, 3});
  }

  {
    const auto sj = DomElementFromString("[]");
    SimdJsonInputArchive ar(sj);
    std::vector<int> a;
    ar.Serialize(a);
    REQUIRE(a == std::vector<int>{});
  }

  {
    const auto sj = DomElementFromString("[123]");
    SimdJsonInputArchive ar(sj);
    std::vector<std::string> a;
    REQUIRE_THROWS_WITH(ar.Serialize(a), ContainsSubstring("index 0:") && ContainsSubstring("INCORRECT_TYPE"));
  }
}
*/

// set?
// custom object
