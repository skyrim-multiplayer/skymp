#pragma once

inline std::string ReadFile(const std::filesystem::path& p)
{
  std::ifstream t(p);
  if (!t.is_open())
    throw std::runtime_error("Unable to open " + p.string() + " for reading");
  std::stringstream content;
  content << t.rdbuf();

  return content.str();
}
