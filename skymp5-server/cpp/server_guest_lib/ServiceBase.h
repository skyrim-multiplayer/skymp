#pragma once
#include <sigslot/signal.hpp>
#include <spdlog/spdlog.h>
#include <vector>

class IService
{
public:
    virtual ~IService() = default;
};

template <class T>
class ServiceBase : public IService
{
public:
    template <class Signal, class Method>
    void On(Signal &signal, Method method) {
        auto self = dynamic_cast<T*>(this);
        if (!self) {
            spdlog::critical("ServiceBase<T>::On called on a non-T instance");
            std::terminate();
        }

        connections.emplace_back(signal.connect_scoped(method, self));
    }

private:
    std::vector<sigslot::scoped_connection> connections;
};
