#pragma once

class SkyrimPlatform
{
public:
  SkyrimPlatform();

private:
  void JsTick(bool gameFunctionsAvailable);

  void TickHello();

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};