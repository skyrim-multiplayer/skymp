#pragma once

#include <memory>
#include <functional>

namespace CEFUtils
{
    struct SKSEPluginBase;

    namespace details
    {
        BOOL ReverseMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved, const std::function<std::unique_ptr<SKSEPluginBase>()>& aAppFactory) noexcept;
    }

    template<class T>
    BOOL CreateReverseApp(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) noexcept
    {
        static_assert(std::is_base_of_v<SKSEPluginBase, T>);

        return details::ReverseMain(hModule, fdwReason, lpReserved, []() { return std::make_unique<T>(); });
    }
}

#define DEFINE_DLL_ENTRY_INITIALIZER(className) \
BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)                              \
{                                                                                                       \
    return CEFUtils::CreateReverseApp<className>(hModule, fdwReason, lpReserved);                  \
}
