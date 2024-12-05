#include "TestUtils.hpp"
#include "archives/JsonOutputArchive.h"
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
#include <vector>

#define MAKE_DOM_ELEMENT_VAR_FROM_STRING(varName, s) \
  std::string tmpString_##varName = s; \
  simdjson::dom::parser parser_##varName; \
  const auto varName = parser_##varName.parse(tmpString_##varName).value();

// MAKE_DOM_ELEMENT_VAR_FROM_STRING(kek, "123");

namespace {
// XXX: remove long double?
using JsonTestParam = std::variant<bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double, long double, std::string>;
using Catch::Matchers::ContainsSubstring;

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

template <class T>
T ParseWithSimdInputArchive(const std::string& json) {
  simdjson::dom::parser parser;
  simdjson::dom::element element = parser.parse(json);
  T result;
  SimdJsonInputArchive ar(element);
  ar.Serialize(result);
  return result;
}

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

  JsonTestParam outVar;
  std::visit([&jsonStr, &outVar](const auto& val) {
    // val is also optional; pick the same slot
    outVar = ParseWithSimdInputArchive<std::decay_t<decltype(val)>>(jsonStr);
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
  {
    // ig this one is fine bc the macro is single-parametred
    REQUIRE(ParseWithSimdInputArchive<std::array<int, 3>>("[1, 2, 3]") == std::array<int, 3>{1, 2, 3});
  }

  {
    REQUIRE_THROWS_WITH((ParseWithSimdInputArchive<std::array<int, 2>>("[1, 2, 3]")), "index 2 out of bounds for output (input is bigger)");
  }

  {
    // option 1
    REQUIRE_THROWS_WITH((ParseWithSimdInputArchive<std::array<int, 4>>("[1, 2, 3]")), "index 3 out of bounds for input (4 elements expected)");
  }

  {
    // option 3
    CheckHelper<std::array<std::string, 3>> h;
    REQUIRE_THROWS_WITH(h.Parse("[1, 2, 3]"), ContainsSubstring("index 0:") && ContainsSubstring("INCORRECT_TYPE"));
  }

  {
    // using TargetType = std::array<std::string, 3>;
    // REQUIRE_THROWS_WITH(ParseWithSimdInputArchive<TargetType>("[]"), "index 0 out of bounds for output");
    using TargetType = std::array<std::string, 3>;
    REQUIRE_THROWS_WITH((ParseWithSimdInputArchive<std::array<std::string, 3>>("[]")), "index 0 out of bounds for input (3 elements expected)");
  }
}

TEST_CASE("SimdJsonArchive vector",
          "[Archives]")
{
  // struct Case {
  //   std::string json;
  //   Matcher
  // };

  {
    REQUIRE(ParseWithSimdInputArchive<std::vector<int>>("[1, 2, 3]") == std::vector<int>{1, 2, 3});
  }

  {
    REQUIRE(ParseWithSimdInputArchive<std::vector<int>>("[]") == std::vector<int>{});
  }

  {
    REQUIRE_THROWS_WITH(ParseWithSimdInputArchive<std::vector<std::string>>("[123]"), ContainsSubstring("index 0:") && ContainsSubstring("INCORRECT_TYPE"));
  }
}

namespace {
struct CustomTestObject {
  int foo{};
  std::optional<std::string> bar{};
  std::optional<float> baz{};

  template <class Archive>
  void Serialize(Archive &ar) {
    ar.Serialize("foo", foo).Serialize("bar", bar).Serialize("baz", baz);
  }
};
}

TEST_CASE("SimdJsonArchive custom",
          "[Archives]")
{
  CustomTestObject obj;
  obj.foo = 123;
  obj.bar = std::nullopt;
  obj.baz = 1.5;

  nlohmann::json nlj;
  {
    JsonOutputArchive ar;
    obj.Serialize(ar);
    nlj = std::move(ar.j);
  }

  auto obj2 = ParseWithSimdInputArchive<CustomTestObject>(nlohmann::to_string(nlj));
  REQUIRE(obj.foo == obj2.foo);
  REQUIRE(obj.bar == obj2.bar);
  REQUIRE(obj.baz == obj2.baz);
}

// set?
// list?
