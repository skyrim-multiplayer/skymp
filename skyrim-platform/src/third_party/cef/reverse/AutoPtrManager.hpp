#pragma once

#include <Pattern.hpp>
#include <mutex>

namespace CEFUtils
{
    struct AutoPtrManager
    {
        TP_NOCOPYMOVE(AutoPtrManager);

        [[nodiscard]] uintptr_t GetBaseAddress() const noexcept;
        [[nodiscard]] void* Find(Pattern aPattern) const noexcept;

        static AutoPtrManager& GetInstance() noexcept
        {
            return s_instance;
        }

    private:

        AutoPtrManager() noexcept;
        ~AutoPtrManager() noexcept;

        uintptr_t m_baseAddress;
        uintptr_t m_textStartAddress;
        size_t m_textSize;
        uint64_t m_textHash;

        static AutoPtrManager s_instance;
    };
}
