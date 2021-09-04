#pragma once

#include <Stl.hpp>

namespace CEFUtils
{
    struct Pattern
    {
        enum EType
        {
            kRelativeIndirection4,
            kDirect
        };

        Pattern(Vector<uint8_t> aBytePattern, size_t aExpected, EType aPatternType, intptr_t aOffset = 0, size_t aIndex = 0) noexcept;

        Vector<uint8_t> BytePattern;
        size_t Expected;
        EType Type;
        intptr_t Offset{ 0 };
        size_t Index{ 0 };
    };
}

namespace std
{
    template<> struct less<CEFUtils::Pattern>
    {
        bool operator() (const CEFUtils::Pattern& lhs, const CEFUtils::Pattern& rhs) const noexcept
        {
            return lhs.BytePattern < rhs.BytePattern;
        }
    };
}
