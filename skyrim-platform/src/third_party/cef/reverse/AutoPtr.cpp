#include <AutoPtr.hpp>
#include <AutoPtrManager.hpp>

#include <cassert>

namespace CEFUtils
{
    BasicAutoPtr::BasicAutoPtr(Pattern aPattern) noexcept
    {
        m_pPtr = AutoPtrManager::GetInstance().Find(std::move(aPattern));

        assert(m_pPtr != nullptr);
    }

    BasicAutoPtr::BasicAutoPtr(const uintptr_t aAddress) noexcept
    {
        m_pPtr = reinterpret_cast<void*>(aAddress + AutoPtrManager::GetInstance().GetBaseAddress());
    }

    void* BasicAutoPtr::GetPtr() const noexcept
    {
        return m_pPtr;
    }
}
