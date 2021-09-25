#pragma once

#include "Allocator.hpp"
#include <new>

namespace CEFUtils
{
    template <class T>
    struct StlAllocator
    {
        using value_type = T;

        StlAllocator()
        {
            m_pAllocator = AllocatorBase::Get();
        }

        template <class U>
        constexpr StlAllocator(const StlAllocator<U>& acRhs) noexcept
        {
            m_pAllocator = acRhs.m_pAllocator;
        }

        [[nodiscard]] T* allocate(std::size_t aSize)
        {
            if (aSize > std::size_t(-1) / sizeof(T))
                throw std::bad_alloc();

            if (auto p = static_cast<T*>(m_pAllocator->Allocate(aSize * sizeof(T))))
                return p;

            throw std::bad_alloc();
        }
        void deallocate(T* p, std::size_t) noexcept
        {
            m_pAllocator->Free(p);
        }


        AllocatorBase* m_pAllocator;
    };

    template <class T, class U>
    bool operator==(const StlAllocator<T>&, const StlAllocator<U>&) { return true; }
    template <class T, class U>
    bool operator!=(const StlAllocator<T>&, const StlAllocator<U>&) { return false; }
}
