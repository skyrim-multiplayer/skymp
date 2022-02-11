#include <GroupUtils.h>
#include <Loader.h>
#include <catch2/catch.hpp>

#include "TestUtils.hpp"
#include "FileInfo.h"

extern espm::Loader l;

// This test is similar to one found in EspmTest.cpp, but check that two ways
// of calculating CRC32 return the same result.
TEST_CASE("FileInfo should return correct checksums of datafiles", "[espm][FileInfo]")
{
  const auto hashes = l.GetHashes();
  const auto sizes = l.GetSizes();
  for (const auto& [filename, checksum] : hashes) {
    DYNAMIC_SECTION(filename << " checksum test")
    {
      const auto fileInfo = FileInfo(GetDataDir() + ('/' + filename));
      REQUIRE(fileInfo.size == sizes.at(filename));
      REQUIRE(static_cast<int32_t>(fileInfo.crc32) == checksum);
    }
  }
}
