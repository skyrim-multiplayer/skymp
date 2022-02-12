#include "FileInfo.h"

#include <cassert>
#include <fstream>
#include <stdexcept>
#include <vector>

#include <zlib.h>

FileInfoResult FileInfo(const std::string& path)
{
  const static size_t kBlockSize = 512 << 10;
  std::vector<char> buf(kBlockSize);

  auto hash = crc32_z(0L, Z_NULL, 0);
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) {
    throw std::runtime_error("FileInfo: can't read " + path);
  }

  size_t size = f.tellg();
  size_t left = size;
  f.seekg(0);

  while (left) {
    size_t now = std::min(left, kBlockSize);
    left -= now;
    if (!f.read(buf.data(), now)) {
      throw std::runtime_error("FileInfo: error during reading " + path);
    }
    hash = crc32_z(hash, reinterpret_cast<const Bytef*>(buf.data()), now);
  }
  assert(f.tellg() == size);

  return { hash, size };
}
