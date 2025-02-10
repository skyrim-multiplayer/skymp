#pragma once
#include "ChangeValuesMessage.h"
#include "DeathStateContainerMessage.h"
#include "OpenContainerMessage.h"
#include "TeleportMessage.h"
#include "UpdateAnimationMessage.h"
#include "UpdateEquipmentMessage.h"
#include "UpdateMovementMessage.h"
#include "UpdatePropertyMessage.h"

#define REGISTER_MESSAGES                                                     \
  REGISTER_MESSAGE(UpdateMovementMessage)                                     \
  REGISTER_MESSAGE(UpdateAnimationMessage)                                    \
  REGISTER_MESSAGE(DeathStateContainerMessage)                                \
  REGISTER_MESSAGE(ChangeValuesMessage)                                       \
  REGISTER_MESSAGE(TeleportMessage)                                           \
  REGISTER_MESSAGE(UpdatePropertyMessage)                                     \
  REGISTER_MESSAGE(OpenContainerMessage)                                      \
  REGISTER_MESSAGE(UpdateEquipmentMessage)
