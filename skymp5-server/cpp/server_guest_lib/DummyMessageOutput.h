#pragma once
#include "IMessageOutput.h"
#include <cstdint>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <vector>

class DummyMessageOutput : public IMessageOutput
{
public:
  void Send(const uint8_t* data, size_t length, bool reliable) override
  {
    auto s = std::string(data + 1, data + length);
    nlohmann::json j;
    try {
      j = nlohmann::json::parse(s);
    } catch (std::exception& e) {
      std::stringstream ss;
      ss << e.what() << " - '" << s << "'";
      throw std::runtime_error(ss.str());
    }
    messages.push_back({ j, reliable });
  }

  struct Message
  {
    nlohmann::json j;
    bool reliable = false;
  };

  std::vector<Message> messages;
};
