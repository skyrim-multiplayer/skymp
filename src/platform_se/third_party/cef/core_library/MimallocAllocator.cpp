#include "MimallocAllocator.hpp"

#include <mimalloc.h>

namespace CEFUtils
{
    void* MimallocAllocator::Allocate(const size_t aSize) noexcept
    {
        return mi_malloc(aSize);
    }

    void MimallocAllocator::Free(void* apData) noexcept
    {
        mi_free(apData);
    }

    size_t MimallocAllocator::Size(void* apData) noexcept
    {
        if (apData == nullptr) return 0;

        return mi_malloc_size(apData);
    }

    void* MimallocAllocator::AlignedAllocate(size_t aSize, size_t aAlignment) noexcept
    {
        return mi_malloc_aligned(aSize, aAlignment);
    }

    void MimallocAllocator::AlignedFree(void* apData) noexcept
    {
        mi_free(apData);
    }
}
