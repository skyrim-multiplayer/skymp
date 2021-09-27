#pragma once

#include <cstddef>
#include <cstdint>

namespace CEFUtils {
namespace FHash {
uint64_t Crc64(const unsigned char* acpData, std::size_t aLength);
}
}
