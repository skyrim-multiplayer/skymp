#pragma once
#include "ActivateMessage.h"
#include "ChangeValuesMessage.h"
#include "DeathStateContainerMessage.h"
#include "OpenContainerMessage.h"
#include "TeleportMessage.h"
#include "UpdateAnimationMessage.h"
#include "UpdateEquipmentMessage.h"
#include "UpdateMovementMessage.h"
#include "UpdatePropertyMessage.h"

#define REGISTER_MESSAGES                                                     \
  REGISTER_MESSAGE(ActivateMessage)                                           \
  REGISTER_MESSAGE(ConsoleCommandMessage)                                     \
  REGISTER_MESSAGE(CraftItemMessage)                                          \
  REGISTER_MESSAGE(CustomEventMessage)                                        \
  REGISTER_MESSAGE(DestroyActorMessage)                                       \
  REGISTER_MESSAGE(DropItemMessage)                                           \
  REGISTER_MESSAGE(FinishSpSnippetMessage)                                    \
  REGISTER_MESSAGE(HitMessage)                                                \
  REGISTER_MESSAGE(HostMessage)                                               \
  REGISTER_MESSAGE(UpdateMovementMessage)                                     \
  REGISTER_MESSAGE(UpdateAnimationMessage)                                    \
  REGISTER_MESSAGE(DeathStateContainerMessage)                                \
  REGISTER_MESSAGE(ChangeValuesMessage)                                       \
  REGISTER_MESSAGE(TeleportMessage)                                           \
  REGISTER_MESSAGE(UpdatePropertyMessage)                                     \
  REGISTER_MESSAGE(OpenContainerMessage)                                      \
  REGISTER_MESSAGE(UpdateEquipmentMessage)
