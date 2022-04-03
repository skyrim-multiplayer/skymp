#pragma once

class InvalidArgumentException : public std::runtime_error
{
public:
  template <class T>
  InvalidArgumentException(const char* varName, const T& value)
    : runtime_error(What(varName, value)){};

private:
  template <class T>
  static std::string What(const char* varName, const T& value)
  {
    std::stringstream ss;
    ss << "'" << value << "' is not a valid argument for '" << varName << "'";
    return ss.str();
  }
};
