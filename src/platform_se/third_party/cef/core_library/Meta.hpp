#pragma once

#include <type_traits>
#include <utility>
#include <cstddef>
#include <functional>


using std::size_t;
namespace CEFUtils
{
    namespace details
    {
        template <class Default, class AlwaysVoid,
            template<class...> class Op, class... Args>
        struct detector {
            using value_t = std::false_type;
            using type = Default;
        };

        template <class Default, template<class...> class Op, class... Args>
        struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
            using value_t = std::true_type;
            using type = Op<Args...>;
        };

        struct nonesuch {

            ~nonesuch() = delete;
            nonesuch(nonesuch const&) = delete;
            void operator=(nonesuch const&) = delete;
        };

        template <template<class...> class Op, class... Args>
        using is_detected = typename detector<nonesuch, void, Op, Args...>::value_t;

        template <template<class...> class Op, class... Args>
        using detected_t = typename detector<nonesuch, void, Op, Args...>::type;

        template< template<class...> class Op, class... Args >
        constexpr bool is_detected_v = is_detected<Op, Args...>::value;

#ifdef _WIN64
        // msvc x64 says the max_align_t is 8, except malloc and alloca always return a 16 byte alignment
        using default_align_t = struct alignas(16) { char _pad[16]; };;
#else
        using default_align_t = std::max_align_t;
#endif
    }

    template<class T>
    void hash_combine(size_t& aSeed, const T& aValue)
    {
        aSeed ^= std::hash<T>()(aValue) + 0x9e3779b9 + (aSeed << 6) + (aSeed >> 2);
    }

    template<class T> struct AlwaysFalse : std::false_type {};
}

#define TP_NOCOPYMOVE(className) \
    className(className&&) = delete; \
    className(const className&) = delete; \
    className& operator=(const className&) = delete; \
    className& operator=(className&&) = delete;
