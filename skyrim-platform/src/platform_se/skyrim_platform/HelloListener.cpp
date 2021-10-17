#include "HelloListener.h"
#include <RE/ConsoleLog.h>

struct HelloListener::Impl
{
  bool helloSaid = false;
};

HelloListener::HelloListener()
{
  pImpl = std::make_shared<Impl>();
}

void HelloListener::Tick()
{
  if (auto console = RE::ConsoleLog::GetSingleton()) {
    if (!pImpl->helloSaid) {
      pImpl->helloSaid = true;
      console->Print("Hello SE");
    }
  }
}

void HelloListener::Update()
{
}

void HelloListener::BeginMain()
{
}

void HelloListener::EndMain()
{
}