#pragma once
#include "DataTypes.h"
#include <cstdint>

namespace espm {

enum RecordFlags : RecordFlagsType
{
  Deleted = 0x00000020,
  Constant = 0x00000040,
  REFR_HiddenFromLocalMap = 0x00000040,
  MustUpdateAnims = 0x00000100,
  REFR_Inaccessible = 0x00000100,
  REFR_HiddenFromLocalMap2 = 0x00000200,
  ACHR_StartsDead = 0x00000200,
  REFR_MotionBlurCastsShadows = 0x00000200,
  QuestItem = 0x00000400,
  PersistentReference = 0x00000400,
  LSCR_DisplaysInMainMenu = 0x00000400,
  InitiallyDisabled = 0x00000800,
  Ignored = 0x00001000,
  VisibleWhenDistant = 0x00008000,
  ACTI_RandomAnimationStart = 0x00010000,
  ACTI_Dangerous = 0x00020000,
  CELL_InteriorOffLimits = 0x00020000,
  Compressed = 0x00040000,
  CantWait = 0x00080000,
  ACTI_IgnoreObjectInteraction = 0x00100000,
  IsMarker = 0x00800000,
  ACTI_Obstacle = 0x02000000,
  REFR_NoAIAcquire = 0x02000000,
  NavMeshGenFilter = 0x04000000,
  NavMeshGenBoundingBox = 0x08000000,
  FURN_MustExitToTalk = 0x10000000,
  REFR_ReflectedByAutoWater = 0x10000000,
  FURN_IDLM_ChildCanUse = 0x20000000,
  REFR_DontHavokSettle = 0x20000000,
  NavMeshGenGround = 0x40000000,
  REFR_NoRespawn = 0x40000000,
  REFR_BultiBound = 0x80000000,
};

}
