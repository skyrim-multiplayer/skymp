#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>
#include <exception>
#include <limits>
#include <nlohmann/json.hpp>
#include <optional>
#include <ostream>
#include <simdjson.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "archives/JsonOutputArchive.h"
#include "archives/SimdJsonInputArchive.h"

namespace {
// XXX: remove long double?
using JsonTestParam = std::variant<bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double, long double, std::string>;
using Catch::Matchers::ContainsSubstring;

template <class T>
T ParseWithSimdInputArchive(const std::string& json) {
  simdjson::dom::parser parser;
  simdjson::dom::element element = parser.parse(json);
  T result;
  SimdJsonInputArchive ar(element);
  ar.Serialize(result);
  return result;
}
} // namespace

namespace Catch {
template<>
struct StringMaker<JsonTestParam> {
  static std::string convert(const JsonTestParam & value ) {
    return std::visit([](auto&& arg) {
      if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, int8_t> || std::is_same_v<std::decay_t<decltype(arg)>, uint8_t>) {
        return std::to_string(arg);
      }
      return StringMaker<decltype(arg)>{}.convert(arg);
    }, value);
  }
};

template<>
struct StringMaker<std::optional<JsonTestParam>> {
  static std::string convert(const std::optional<JsonTestParam> & value ) {
    if (!value) {
      return "nullopt";
    }
    return StringMaker<JsonTestParam>{}.convert(*value);
  }
};
} // namespace Catch

TEST_CASE("SimdJsonArchive simple",
          "[Archives]")
{
  JsonTestParam param = GENERATE(
    // one param is enough, type will be deduced for the others
    JsonTestParam("test_string"),
    false,
    true,
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
    13.37f,  // TODO: check with eps if needed
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
    outVar = ParseWithSimdInputArchive<std::decay_t<decltype(val)>>(jsonStr);
  }, param);
  REQUIRE(param.index() == outVar.index());
  REQUIRE(param == outVar);
}

namespace {
template <class T>
struct TypeMarker {
  T Get() const;

  using type = T;
};

template <class T>
std::ostream& operator<<(std::ostream& os, const TypeMarker<T>&) {
  return os << typeid(T).name();
}

using TypeMarkerVariant = std::variant<TypeMarker<uint8_t>, TypeMarker<int8_t>, TypeMarker<uint16_t>, TypeMarker<int16_t>, TypeMarker<uint32_t>, TypeMarker<int32_t>, TypeMarker<uint64_t>, TypeMarker<int64_t>>;

std::ostream& operator<<(std::ostream& os, const TypeMarkerVariant& marker) {
  return std::visit([&os](const auto& markerVal) -> std::ostream& {
    return os << markerVal;
  }, marker);
}

bool IsSigned(const TypeMarkerVariant& marker) {
  return std::visit([](const auto& markerVal) {
    return std::is_signed_v<decltype(markerVal.Get())>;
  }, marker);
}

template <class T>
bool InBounds(const TypeMarkerVariant& type, T num) {
  return std::visit([num](const auto& markerVal) {
    using Limits = std::numeric_limits<decltype(markerVal.Get())>;
    return Limits::min() <= num && num <= Limits::max();
  }, type);
}

std::optional<JsonTestParam> ParseInt(const TypeMarkerVariant& type, const std::string& s) {
  JsonTestParam result;
  try {
    if (IsSigned(type)) {
      auto num = std::stoll(s);
      if (!InBounds(type, num)) {
        return std::nullopt;
      }
      return num;
    } else {
      if (s.length() && s[0] == '-') {
        return std::nullopt;
      }
      auto num = std::stoull(s);
      if (!InBounds(type, num)) {
        return std::nullopt;
      }
      return num;
    }
  } catch (const std::out_of_range&) {
    return std::nullopt;
  }
}

JsonTestParam ParseWithSimdInputArchiveMarker(const TypeMarkerVariant& type, const std::string& json) {
  return std::visit([&json](const auto& typeVal) -> JsonTestParam {
    return ParseWithSimdInputArchive<typename std::decay_t<decltype(typeVal)>::type>(json);
  }, type);
}

}

TEST_CASE("SimdJsonArchive overflow test util",
          "[Archives]")
{
  // tests the simple check against stdd::numeric_limits
  REQUIRE(std::get<uint64_t>(ParseInt(TypeMarker<uint8_t>(), "255").value()) == 255);
  REQUIRE(ParseInt(TypeMarker<int8_t>(), "255") == std::nullopt);
  REQUIRE(std::get<int64_t>(ParseInt(TypeMarker<int8_t>(), "-128").value()) == -128);
  REQUIRE(ParseInt(TypeMarker<uint8_t>(), "-128") == std::nullopt);

  // tests unsigned overflow checks
  REQUIRE(ParseInt(TypeMarker<uint64_t>(), "-1") == std::nullopt);
  REQUIRE(ParseInt(TypeMarker<int64_t>(), std::to_string(static_cast<uint64_t>(-1))) == std::nullopt);
}

TEST_CASE("SimdJsonArchive overflow",
          "[Archives]")
{
  auto typeMarker = GENERATE(
    TypeMarkerVariant(TypeMarker<uint8_t>()),
    TypeMarkerVariant(TypeMarker< int8_t>()),
    TypeMarkerVariant(TypeMarker<uint16_t>()),
    TypeMarkerVariant(TypeMarker< int16_t>()),
    TypeMarkerVariant(TypeMarker<uint32_t>()),
    TypeMarkerVariant(TypeMarker< int32_t>()),
    TypeMarkerVariant(TypeMarker<uint64_t>()),
    TypeMarkerVariant(TypeMarker< int64_t>())
  );
  CAPTURE(typeMarker);

  auto numStr = GENERATE(
    std::string{"0"},
    "1",
    "-1",
    std::to_string(static_cast<uint64_t>(std::numeric_limits<int8_t>::max()) + 1),
    std::to_string(static_cast<uint64_t>(std::numeric_limits<int16_t>::max()) + 1),
    std::to_string(static_cast<uint64_t>(std::numeric_limits<int32_t>::max()) + 1),
    std::to_string(static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1),
    std::to_string(static_cast<int64_t>(std::numeric_limits<int32_t>::min()) - 1),
    "999999999999999999999999999999999999",
    "-999999999999999999999999999999999999"
  );
  CAPTURE(numStr);

  auto numVariant = ParseInt(typeMarker, numStr);
  CAPTURE(numVariant);

  if (numVariant) {
    auto parsed = ParseWithSimdInputArchiveMarker(typeMarker, numStr);
    CAPTURE(parsed);
    auto parsedToStr = std::visit([](const auto& v) -> std::string { 
      if constexpr (std::is_integral_v<std::decay_t<decltype(v)>>) {
        return std::to_string(v);
      }
      return "not integral, something wrong";
    }, parsed);
    REQUIRE(parsedToStr == numStr);
    // FAIL();
  } else {
    try {
      auto parsed = ParseWithSimdInputArchiveMarker(typeMarker, numStr);
      CAPTURE(parsed);
      FAIL("expected to throw exception");
    } catch (const std::exception& e) {
      // FAIL(e.what());
    }
  }
}

TEST_CASE("SimdJsonArchive array",
          "[Archives]")
{
  REQUIRE(ParseWithSimdInputArchive<std::array<int, 3>>("[1, 2, 3]") == std::array<int, 3>{1, 2, 3});
  REQUIRE_THROWS_WITH((ParseWithSimdInputArchive<std::array<int, 2>>("[1, 2, 3]")), "index 2 out of bounds for output (input is bigger)");
  REQUIRE_THROWS_WITH((ParseWithSimdInputArchive<std::array<int, 4>>("[1, 2, 3]")), "index 3 out of bounds for input (4 elements expected)");
  REQUIRE_THROWS_WITH((ParseWithSimdInputArchive<std::array<std::string, 3>>("[1, 2, 3]")), ContainsSubstring("index 0:") && ContainsSubstring("INCORRECT_TYPE"));
  REQUIRE_THROWS_WITH((ParseWithSimdInputArchive<std::array<int, 3>>("[]")), "index 0 out of bounds for input (3 elements expected)");
}

TEST_CASE("SimdJsonArchive vector",
          "[Archives]")
{
  REQUIRE(ParseWithSimdInputArchive<std::vector<int>>("[1, 2, 3]") == std::vector<int>{1, 2, 3});
  REQUIRE(ParseWithSimdInputArchive<std::vector<int>>("[]") == std::vector<int>{});
  REQUIRE_THROWS_WITH(ParseWithSimdInputArchive<std::vector<std::string>>("[123]"), ContainsSubstring("index 0:") && ContainsSubstring("INCORRECT_TYPE"));
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
    nlj = std::move(ar.j); // TODO(#2250): should be a method instead. TakeValue/TakeOutput?
  }

  auto obj2 = ParseWithSimdInputArchive<CustomTestObject>(nlohmann::to_string(nlj));
  REQUIRE(obj.foo == obj2.foo);
  REQUIRE(obj.bar == obj2.bar);
  REQUIRE(obj.baz == obj2.baz);
}

// set?
// list?
