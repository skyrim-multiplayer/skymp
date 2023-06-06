#include "FileUtils.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

std::string Viet::ReadFileIntoString(const std::filesystem::path& filePath)
{
  std::ifstream file(filePath);

  if (!file.is_open()) {
    throw std::runtime_error("Unable to open file '" + filePath.string() +
                             "' for reading");
  }

  std::stringstream resultString;
  resultString << file.rdbuf();

  return resultString.str();
}
