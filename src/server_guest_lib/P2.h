#pragma once
#include "P2SessionManager.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

class P2
{
public:
  P2()
  {
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
    auto logger = std::make_shared<spdlog::logger>("log", sink);

    listeners.push_back(
      std::shared_ptr<P2SessionManager>(new P2SessionManager(logger)));
  }

  static void Attach(std::shared_ptr<PartOne> partOne,
                     std::shared_ptr<P2> partTwo)
  {
    for (auto& listener : partTwo->listeners)
      partOne->AddListener(listener);
  }

private:
  std::vector<std::shared_ptr<PartOne::Listener>> listeners;
};