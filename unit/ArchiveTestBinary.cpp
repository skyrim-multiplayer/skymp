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
