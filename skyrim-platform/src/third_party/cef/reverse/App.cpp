#include <App.hpp>

namespace CEFUtils
{
    SKSEPluginBase::SKSEPluginBase() noexcept
        : m_ready(false)
    {
    }

    SKSEPluginBase::~SKSEPluginBase() = default;

    void SKSEPluginBase::Start() noexcept
    {
        m_ready = true;

        BeginMain();
    }

    bool SKSEPluginBase::IsReady() const noexcept
    {
        return m_ready;
    }
}
