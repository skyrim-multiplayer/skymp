#pragma once

class SkyrimPlatform
{
public:
  SkyrimPlatform();
  void JsTick(bool gameFunctionsAvailable);

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};