#pragma once

#include <Stl.hpp>
#include <filesystem>

namespace CEFUtils {
std::filesystem::path GetPath() noexcept;
String LoadFile(const std::filesystem::path& acPath) noexcept;
}
