#pragma once

#include <string_view>

bool ValidateFilename(std::string_view filename, bool allowDots);

bool ValidateRelativePath(std::string_view path);
