#pragma once
#include <stdexcept>
#include <string>
#include <zlib.h>

inline void ZlibDecompress(const void* in, size_t inSize, void* out,
                           size_t outSize)
{
  z_stream infstream;
  infstream.zalloc = Z_NULL;
  infstream.zfree = Z_NULL;
  infstream.opaque = Z_NULL;

  infstream.avail_in = (uInt)(inSize); // size of input
  infstream.next_in = (Bytef*)in;      // input char array
  infstream.avail_out = (uInt)outSize; // size of output
  infstream.next_out = (Bytef*)out;    // output char array

  inflateInit(&infstream);

  int res = inflate(&infstream, Z_NO_FLUSH);
  if (res < Z_OK)
    throw std::runtime_error("inflate() failed with code " +
                             std::to_string(res));
  res = inflateEnd(&infstream);
  if (res < Z_OK)
    throw std::runtime_error("inflateEnd() failed with code " +
                             std::to_string(res));
}

inline size_t ZlibCompress(const void* in, size_t inSize, void* out,
                           size_t outMaxSize)
{
  z_stream defstream;
  defstream.zalloc = Z_NULL;
  defstream.zfree = Z_NULL;
  defstream.opaque = Z_NULL;

  defstream.avail_in = (uInt)inSize;
  defstream.next_in = (Bytef*)in;         // input char array
  defstream.avail_out = (uInt)outMaxSize; // size of output
  defstream.next_out = (Bytef*)out;       // output char array

  // the actual compression work.
  deflateInit(&defstream, Z_BEST_COMPRESSION);
  int res = deflate(&defstream, Z_FINISH);
  if (res < Z_OK)
    throw std::runtime_error("deflate() failed with code " +
                             std::to_string(res));
  const auto outputSize = defstream.next_out - (uint8_t*)out;
  res = deflateEnd(&defstream);
  if (res < Z_OK)
    throw std::runtime_error("deflateEnd() failed with code " +
                             std::to_string(res));
  return outputSize;
}

inline uLong ZlibGetCRC32Checksum(const void* readBuffer, z_size_t length)
{
  auto hash = crc32_z(0L, Z_NULL, 0);
  hash = crc32_z(hash, static_cast<const Bytef*>(readBuffer), length);
  return hash;
}
