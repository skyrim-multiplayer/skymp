#pragma once
#include "TPOverlayService.h"
#include <functional>
#include <memory>

class SkyrimPlatform
{
public:
  static SkyrimPlatform& GetSingleton();

  void JsTick(bool gameFunctionsAvailable);
  void SetOverlayService(std::shared_ptr<OverlayService> overlayService);
  void AddTickTask(std::function<void()> f);
  void AddUpdateTask(std::function<void()> f);

private:
  SkyrimPlatform();
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};