#include <catch2/catch_all.hpp>
#include <libespm/GroupUtils.h>
#include <libespm/Loader.h>

#include "../TestUtils.hpp"
#include "FileInfo.h"

extern espm::Loader l;

// This test is similar to one found in EspmTest.cpp, but check that two ways
// of calculating CRC32 return the same result.
TEST_CASE("FileInfo should return correct checksums of datafiles",
          "[espm][FileInfo]")
{
  const auto files = l.GetFilesInfo();
  for (const auto& [filename, espmInfo] : files) {
    DYNAMIC_SECTION(filename << " checksum test")
    {
      const auto fileInfo = FileInfo(GetDataDir() + ('/' + filename));
      REQUIRE(fileInfo.size == espmInfo.size);
      REQUIRE(fileInfo.crc32 == espmInfo.crc32);
    }
  }
}
