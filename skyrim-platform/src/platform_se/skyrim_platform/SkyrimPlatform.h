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
  void AddTickTask(const std::function<void()>& f);
  void AddUpdateTask(const std::function<void()>& f);
  void PushAndWait(const std::function<void(int)>& task);

private:
  SkyrimPlatform();
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
