#pragma once
#include "Networking.h"

class Bot
{
public:
  Bot(std::shared_ptr<Networking::IClient> cl_)
    : cl(cl_)
  {
  }

  void Destroy() { cl.reset(); }

  void Send(std::string packet)
  {
    if (cl)
      cl->Send(reinterpret_cast<Networking::PacketData>(packet.data()),
               packet.size(), true);
  }

  void Tick()
  {
    if (cl)
      cl->Tick([](auto, auto, auto, auto, auto) {}, nullptr);
  }

private:
  std::shared_ptr<Networking::IClient> cl;
};
