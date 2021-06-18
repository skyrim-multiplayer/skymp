#pragma once
#include "Networking.h"

class GridElement
{
public:
  static GridElement NewProducer(uint32_t formId)
  {
    return {formId, true};
  }

  static GridElement NewConsumer(Networking::UserId userId)
  {
    return {userId, false};
  }

  bool IsProducer() const noexcept { return isProducer; }

  bool IsConsumer() const noexcept { return !IsProducer(); }

private:
  const uint32_t formIdOrUserId;
  const bool isProducer;

  GridElement(uint32_t formIdOrUserId_, bool isProducer_)
    : formIdOrUserId(formIdOrUserId_)
    , isProducer(isProducer_)
  {
  }
};