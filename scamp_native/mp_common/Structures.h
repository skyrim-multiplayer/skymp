#pragma once

enum class RunMode
{
  Standing,
  Walking,
  Running,
  Sprinting
};

enum class MovementFlags
{
  RunMode0 = 0,
  RunMode1 = 1 << 0,
  IsInJumpState = 1 << 1,
  IsSneaking = 1 << 2,
  IsBlocking = 1 << 3,
  IsWeapDrawn = 1 << 4
};

enum class MovementDirection
{
  Left,
  Right,
  Forward,
  Back,
  RotateInPlace,
};

struct Movement
{
  int x, y, z;                 // 4 bytes at var
  short int angleZ;            // 2 bytes
  MovementDirection direction; // 4 bytes
  MovementFlags movementFlags; // 4 bytes
  int worldOrCell;             // 4 bytes
};                             // total 26 bytes
