#pragma warning(disable: 4073)
#pragma init_seg(lib)

#include <AutoPtrManager.hpp>
#include <Hash.hpp>

#include <windows.h>

#include <algorithm>
#include <cassert>

namespace CEFUtils
{
    AutoPtrManager AutoPtrManager::s_instance;

    AutoPtrManager::AutoPtrManager() noexcept
        : m_baseAddress(0), m_textStartAddress(0), m_textSize(0)
    {
        m_baseAddress = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));

        const auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(m_baseAddress + static_cast<uintptr_t>(reinterpret_cast<
            PIMAGE_DOS_HEADER>(m_baseAddress)->e_lfanew));

        auto pSection = reinterpret_cast<const IMAGE_SECTION_HEADER*>(ntHeaders + 1);
        for (auto i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i, ++pSection)
        {
            if (strcmp(reinterpret_cast<const char*>(pSection->Name), ".text") == 0)
            {
                m_textStartAddress = m_baseAddress + pSection->VirtualAddress;
                m_textSize = pSection->SizeOfRawData;
                break;
            }
        }

        m_textHash = FHash::Crc64(reinterpret_cast<const unsigned char*>(m_textStartAddress), m_textSize);
    }

    AutoPtrManager::~AutoPtrManager() noexcept = default;

    uintptr_t AutoPtrManager::GetBaseAddress() const noexcept
    {
        return m_baseAddress;
    }

    void* AutoPtrManager::Find(Pattern aPattern) const noexcept
    {
        const auto* cstart = reinterpret_cast<const uint8_t*>(m_textStartAddress);
        const auto cend = cstart + m_textSize;

        const auto cmpOp = [](uint8_t a, uint8_t b)
        {
            return (a == b || b == 0xCC);
        };

        size_t i = 0;
        while (true)
        {
            const auto res = std::search(cstart, cend, aPattern.BytePattern.begin(), aPattern.BytePattern.end(), cmpOp);
            if (res >= cend)
                break;

            if (aPattern.Index == i)
            {
                if (aPattern.Type == Pattern::kRelativeIndirection4)
                {
                    const auto address = res + aPattern.Offset;
                    return (void*)(address + *reinterpret_cast<const int32_t*>(address) + 4);
                }
                return (void*)(res + aPattern.Offset);

            }

            ++i;

            cstart = res + aPattern.BytePattern.size();
        }

        return nullptr;
    }
}
