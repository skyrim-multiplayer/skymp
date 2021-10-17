#pragma once
#include "SkyrimPlatformListener.h"

class HelloListener : public SkyrimPlatformListener
{
public:
  HelloListener();

  void Tick() override;
  void Update() override;
  void BeginMain() override;
  void EndMain() override;

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};