#pragma once
#include <cstdint>

enum class MsgType : int64_t
{
  Invalid = 0,
  CustomPacket = 1,
  UpdateMovement = 2,
  UpdateAnimation = 3
};