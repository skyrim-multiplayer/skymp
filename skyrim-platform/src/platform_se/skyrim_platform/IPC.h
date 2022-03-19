#pragma once

namespace IPC {

typedef void (*MessageCallback)(const uint8_t* data, uint32_t length,
                                void* state);

class CallbackData
{
public:
  CallbackData() = default;
  CallbackData(const std::string& _systemName,
               const MessageCallback& _callback, void* _state)
    : systemName(_systemName)
    , callback(_callback)
    , state(_state)
  {
  }

  friend bool operator==(const CallbackData& lhs, const CallbackData& rhs)
  {
    return std::make_tuple(lhs.systemName, lhs.callback, lhs.state) ==
      std::make_tuple(rhs.systemName, rhs.callback, rhs.state);
  }

  const std::string systemName;
  const MessageCallback callback;
  void* state = nullptr;
};

uint32_t Subscribe(const char* systemName, MessageCallback callback,
                   void* state);

void Unsubscribe(uint32_t subscriptionId);

void Call(const std::string& systemName, const uint8_t* data, uint32_t length);
void Send(const char* systemName, const uint8_t* data, uint32_t length);
}
