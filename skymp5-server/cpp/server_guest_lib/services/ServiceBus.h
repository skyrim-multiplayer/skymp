#pragma once

#include <memory>

namespace Networking {
class IServer;
}

class GlobalObjectsService;
class MetricsService;

class ServiceBus
{
public:
  template <class T>
  std::shared_ptr<T> Get() const {
    static_assert(false, "service is not available in the service bus");
  }

  template <>
  std::shared_ptr<GlobalObjectsService> Get<GlobalObjectsService>() const;

  template <>
  std::shared_ptr<MetricsService> Get<MetricsService>() const;
};
