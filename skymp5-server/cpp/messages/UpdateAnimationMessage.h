#pragma once
#include <cstdint>
#include <string>

struct UpdateAnimationMessage {
  const static char kHeaderByte = 'A';

  uint32_t idx = 0;
  uint32_t numChanges = 0;
  std::string animEventName;
};
