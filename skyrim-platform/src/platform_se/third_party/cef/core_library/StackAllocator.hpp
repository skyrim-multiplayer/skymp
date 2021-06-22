#pragma once

#include "Allocator.hpp"
#include <memory>

namespace CEFUtils
{
    template <size_t Bytes>
    struct StackAllocator : AllocatorBase
    {
        StackAllocator() noexcept;
        virtual ~StackAllocator();

        TP_NOCOPYMOVE(StackAllocator);

        [[nodiscard]] void* Allocate(size_t aSize) noexcept override;
        void Free(void* apData) noexcept override;
        [[nodiscard]] size_t Size(void* apData) noexcept override;

    private:

        size_t m_size;
        void* m_pCursor;
        char m_data[Bytes];
    };

    template <size_t Bytes>
    StackAllocator<Bytes>::StackAllocator() noexcept
        : m_size(Bytes)
    {
        m_pCursor = static_cast<void*>(m_data);
    }

    template <size_t Bytes>
    StackAllocator<Bytes>::~StackAllocator() = default;

    template <size_t Bytes>
    void* StackAllocator<Bytes>::Allocate(const size_t aSize) noexcept
    {
        if (std::align(alignof(std::max_align_t), aSize, m_pCursor, m_size))
        {
            const auto pResult = m_pCursor;
            m_pCursor = static_cast<char*>(m_pCursor) + aSize;
            m_size -= aSize;
            return pResult;
        }

        return nullptr;
    }

    template <size_t Bytes>
    void StackAllocator<Bytes>::Free(void* apData) noexcept
    {
        // do nothing here
        (void)apData;
    }

    template <size_t Bytes>
    size_t StackAllocator<Bytes>::Size(void* apData) noexcept
    {
        (void)apData;
        return Bytes;
    }
}
