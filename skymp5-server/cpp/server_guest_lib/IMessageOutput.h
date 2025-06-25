#pragma once

class IMessageBase;

class IMessageOutput
{
public:
  virtual ~IMessageOutput() = default;

  virtual void Send(const IMessageBase& message, bool reliable) = 0;
};
