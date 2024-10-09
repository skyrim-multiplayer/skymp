#pragma once

struct AnimationGraphDescriptor final
{
    template <std::size_t N, std::size_t O, std::size_t P> AnimationGraphDescriptor(const uint32_t (&acBooleanList)[N], const uint32_t (&acFloatList)[O], const uint32_t (&acIntegerList)[P])
    {
        static_assert(N <= 64, "Too many boolean variables!");
        static_assert((1 + O + P) <= 64, "Too many float and integer!");

        BooleanLookUpTable.assign(acBooleanList, acBooleanList + N);
        FloatLookupTable.assign(acFloatList, acFloatList + O);
        IntegerLookupTable.assign(acIntegerList, acIntegerList + P);
    }

    std::vector<uint32_t> BooleanLookUpTable;
    std::vector<uint32_t> FloatLookupTable;
    std::vector<uint32_t> IntegerLookupTable;
};
