#include <Pattern.hpp>

namespace CEFUtils
{
    Pattern::Pattern(Vector<uint8_t> aBytePattern, const size_t aExpected, const EType aPatternType, const intptr_t aOffset /* = 0 */, const size_t aIndex /* = 0 */) noexcept
        : BytePattern(std::move(aBytePattern))
        , Expected(aExpected)
        , Type(aPatternType)
        , Offset(aOffset)
        , Index(aIndex)
    {
    }
}
