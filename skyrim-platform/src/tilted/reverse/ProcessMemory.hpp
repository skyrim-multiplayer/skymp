#pragma once

#include <AutoPtr.hpp>
#include <type_traits>

namespace CEFUtils
{
    struct ProcessMemory
    {
        ProcessMemory(void* apMemoryLocation, size_t aSize) noexcept;
        ~ProcessMemory();

        TP_NOCOPYMOVE(ProcessMemory);

        template<class T, typename = std::enable_if_t<std::is_pod<T>::value>>
        bool Write(const T & acData, const size_t aOffset = 0) const noexcept
        {
            return WriteBuffer(reinterpret_cast<const unsigned char*>(&acData), sizeof(T), aOffset);
        }

        bool WriteBuffer(const unsigned char* acpData, size_t aSize, size_t aOffset) const noexcept;

    private:

        unsigned long m_oldProtect;
        void* m_pMemoryLocation;
        size_t m_size;
    };

    template<typename TVar, typename TAddress>
    bool Write(const TAddress acAddress, const TVar acVal, const size_t acOffset = 0)
    {
        AutoPtr<TVar> ptr(static_cast<uintptr_t>(acAddress));

        ProcessMemory memory(ptr.GetPtr(), sizeof(TVar));
        return memory.Write(acVal, acOffset);
    }

    template<typename TAddress>
    bool Nop(const TAddress acAddress, size_t aLength)
    {
        AutoPtr<TAddress> ptr(static_cast<uintptr_t>(acAddress));

        const ProcessMemory memory(ptr.GetPtr(), aLength);
        return memory.WriteBuffer(reinterpret_cast<const unsigned char*>(ptr.GetPtr()), aLength, 0);
    }
}
