#pragma once

#include <Pattern.hpp>

namespace CEFUtils
{
    struct BasicAutoPtr
    {
        explicit BasicAutoPtr(Pattern aPattern) noexcept;
        explicit BasicAutoPtr(uintptr_t aAddress) noexcept;

        BasicAutoPtr() = delete;
        BasicAutoPtr(BasicAutoPtr&) = delete;
        BasicAutoPtr& operator=(BasicAutoPtr&) = delete;

        [[nodiscard]] void* GetPtr() const noexcept;

    private:

        void* m_pPtr;
    };

    template<class T>
    struct AutoPtr : BasicAutoPtr
    {
        explicit AutoPtr(Pattern aPattren) noexcept : BasicAutoPtr(std::move(aPattren)) {}
        explicit AutoPtr(const uintptr_t aAddress) noexcept : BasicAutoPtr(aAddress) {}

        AutoPtr() = delete;
        AutoPtr(AutoPtr&) = delete;
        AutoPtr& operator=(AutoPtr&) = delete;

        operator T* () const noexcept { return Get(); }
        T* operator->() const noexcept { return Get(); }

        T* Get() const noexcept { return static_cast<T*>(GetPtr()); }
    };
}
