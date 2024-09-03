#pragma once
#include <concepts>
#include <type_traits>

#include "Arithmetic.h"
#include "ContainerLike.h"
#include "IntegralConstant.h"
#include "Optional.h"
#include "StringLike.h"

template <typename T>
concept NoneOfTheAbove = !IntegralConstant<T> && !StringLike<T> &&
  !ContainerLike<T> && !Optional<T> && !Arithmetic<T>;
