#pragma once
#include <cstdint>

enum class MsgType : int64_t
{
  Invalid = 0,
  CustomPacket = 1,
  UpdateMovement = 2,
  UpdateAnimation = 3,
  UpdateLook = 4,
  UpdateEquipment = 5,
  Activate = 6,
  UpdateProperty = 7,
};