#include <catch2/catch_all.hpp>
#include <exception>
#include <limits>
#include <nlohmann/json.hpp>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "archives/BitStreamInputArchive.h"
#include "archives/BitStreamOutputArchive.h"

namespace {
struct VariantStruct
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("args", args);
  }

  std::vector<std::variant<int32_t, std::string>> args;
};
}

TEST_CASE("BitStreamArchive variant", "[Archives] [Serialization]")
{
  VariantStruct variantStruct;
  variantStruct.args.push_back(255);
  variantStruct.args.push_back("a");
  variantStruct.args.push_back(2);
  variantStruct.args.push_back("b");
  variantStruct.args.push_back(3);
  variantStruct.args.push_back("c");

  SLNet::BitStream stream;

  BitStreamOutputArchive archive(stream);
  variantStruct.Serialize(archive);

  std::vector<uint8_t> data = { static_cast<uint8_t*>(stream.GetData()),
                                static_cast<uint8_t*>(stream.GetData()) +
                                  stream.GetNumberOfBytesUsed() };

  REQUIRE(data ==
          std::vector<uint8_t>{
            0,   0, 0, 6,   // vector size
            0,   0, 0, 0,   // variant index 0 (int)
            0,   0, 0, 255, // 255
            0,   0, 0, 1,   // variant index 1 (string)
            0,   0, 0, 1,   // string length 1
            'a',            // character 'a'
            0,   0, 0, 0,   // variant index 0 (int)
            0,   0, 0, 2,   // 2
            0,   0, 0, 1,   // variant index 1 (string)
            0,   0, 0, 1,   // string length 1
            'b',            // character 'b'
            0,   0, 0, 0,   // variant index 0 (int)
            0,   0, 0, 3,   // 3
            0,   0, 0, 1,   // variant index 1 (string)
            0,   0, 0, 1,   // string length 1
            'c'             // character 'c'
          });

  SLNet::BitStream streamRead;

  VariantStruct variantStructRead;

  BitStreamInputArchive archiveRead(stream);
  variantStructRead.Serialize(archiveRead);

  REQUIRE(variantStructRead.args == variantStruct.args);
}

using ExtendedVariant = std::variant<int32_t, std::string, double, bool>;
struct ExtendedVariantStruct
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("args", args);
  }
  std::vector<ExtendedVariant> args;
};

using NestedVariant = std::variant<int, std::variant<std::string, double>>;
struct NestedVariantStruct
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("args", args);
  }
  std::vector<NestedVariant> args;
};

TEST_CASE("BitStreamArchive variant - extended types and edge cases",
          "[Archives] [Serialization]")
{
  ExtendedVariantStruct extStruct;
  extStruct.args.push_back(42);
  extStruct.args.push_back("hello");
  extStruct.args.push_back(3.14);
  extStruct.args.push_back(true);
  extStruct.args.push_back(false);

  SLNet::BitStream stream;
  BitStreamOutputArchive archive(stream);
  extStruct.Serialize(archive);

  SLNet::BitStream streamRead;
  streamRead.Write(reinterpret_cast<const char*>(stream.GetData()),
                   stream.GetNumberOfBytesUsed());
  ExtendedVariantStruct extStructRead;
  BitStreamInputArchive archiveRead(streamRead);
  extStructRead.Serialize(archiveRead);
  REQUIRE(extStructRead.args == extStruct.args);

  // Nested variant
  std::vector<NestedVariant> nestedArgs = {
    1, std::variant<std::string, double>("nested"),
    std::variant<std::string, double>(2.71)
  };
  NestedVariantStruct nestedStruct{ nestedArgs };

  SLNet::BitStream stream2;
  BitStreamOutputArchive archive2(stream2);
  nestedStruct.Serialize(archive2);

  SLNet::BitStream stream2Read;
  stream2Read.Write(reinterpret_cast<const char*>(stream2.GetData()),
                    stream2.GetNumberOfBytesUsed());
  NestedVariantStruct nestedStructRead;
  BitStreamInputArchive archive2Read(stream2Read);
  nestedStructRead.Serialize(archive2Read);
  REQUIRE(nestedStructRead.args == nestedStruct.args);
}
