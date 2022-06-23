#pragma once

#include <string>

struct FileInfoResult
{
  size_t crc32;
  size_t size;
};

FileInfoResult FileInfo(const std::string& path);
