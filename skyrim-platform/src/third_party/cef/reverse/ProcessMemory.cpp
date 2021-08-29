#include <ProcessMemory.hpp>

#include <Windows.h>
#include <algorithm>

namespace CEFUtils
{
    ProcessMemory::ProcessMemory(void* apMemoryLocation, const size_t aSize) noexcept
        : m_oldProtect(0)
        , m_pMemoryLocation(apMemoryLocation)
        , m_size(aSize)
    {
        VirtualProtect(m_pMemoryLocation, m_size, PAGE_EXECUTE_READWRITE, &m_oldProtect);
    }

    ProcessMemory::~ProcessMemory()
    {
        VirtualProtect(m_pMemoryLocation, m_size, m_oldProtect, &m_oldProtect);
    }

    bool ProcessMemory::WriteBuffer(const unsigned char* acpData, const size_t aSize, const size_t aOffset) const noexcept
    {
        if (aSize + aOffset > m_size)
            return false;

        auto* pMemory = reinterpret_cast<unsigned char*>(m_pMemoryLocation) + aOffset;
        std::copy(acpData, acpData + aSize, pMemory);

        return true;
    }
}
