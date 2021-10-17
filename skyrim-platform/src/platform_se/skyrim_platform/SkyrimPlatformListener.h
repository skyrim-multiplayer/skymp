#pragma once

class SkyrimPlatformListener
{
public:
  virtual ~SkyrimPlatformListener() = default;
  virtual void Tick() = 0;
  virtual void Update() = 0;
  virtual void BeginMain() = 0;
  virtual void EndMain() = 0;
};