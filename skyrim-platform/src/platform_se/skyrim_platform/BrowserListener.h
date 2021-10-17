#pragma once
#include "BrowserApi.h"
#include "SkyrimPlatformListener.h"

class BrowserListener : public SkyrimPlatformListener
{
public:
  BrowserListener(std::shared_ptr<BrowserApi::State> browserApiState);

  void Tick() override;
  void Update() override;
  void BeginMain() override;
  void EndMain() override;

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};