#pragma once

class NullPointerException : public std::runtime_error
{
public:
  NullPointerException(const char* varName)
    : runtime_error(What(varName)){};

private:
  static std::string What(const char* varName)
  {
    std::stringstream ss;
    ss << "Expected '" << varName << "' to not to be nullptr";
    return ss.str();
  }
};
