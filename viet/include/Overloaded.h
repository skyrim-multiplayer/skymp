#pragma once
// See https://en.cppreference.com/w/cpp/utility/variant/visit

namespace Viet {
template <class... Ts>
struct Overloaded : Ts...
{
  using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;
}
