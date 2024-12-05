#include "TestUtils.hpp"
#include "archives/SimdJsonInputArchive.h"
#include <catch2/catch_all.hpp>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/internal/catch_to_string.hpp>
#include <exception>
#include <nlohmann/detail/conversions/to_json.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <simdjson.h>
#include <string>
#include <variant>

namespace {
using JsonTestParam = std::variant<bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double, long double, std::string>;
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
    13.37L, // long double; not sure if it should be tested or not. We'd lose precision here anyway
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

  // FAIL();
}

// object
// array
// vector
// set?
// custom object
