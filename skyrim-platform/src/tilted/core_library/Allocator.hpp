#pragma once

#include "Meta.hpp"

#define TP_ALLOCATOR                                                          \
  void SetAllocator(AllocatorBase* apAllocator) noexcept                      \
  {                                                                           \
    m_pAllocator = apAllocator;                                               \
  }                                                                           \
  CEFUtils::AllocatorBase* GetAllocator() noexcept { return m_pAllocator; }   \
                                                                              \
private:                                                                      \
  CEFUtils::AllocatorBase* m_pAllocator{ CEFUtils::AllocatorBase::Get() };    \
                                                                              \
public:

namespace CEFUtils {
namespace details {
template <typename T>
using has_get_allocator_t = decltype(std::declval<T&>().GetAllocator());

template <typename T>
using has_set_allocator_t = decltype(std::declval<T&>().SetAllocator(nullptr));

template <typename T>
constexpr bool has_allocator = is_detected_v<has_get_allocator_t, T>&&
  is_detected_v<has_set_allocator_t, T>;
}

struct AllocatorBase
{
  virtual ~AllocatorBase() = default;
  [[nodiscard]] virtual void* Allocate(size_t aSize) noexcept = 0;
  virtual void Free(void* apData) noexcept = 0;
  [[nodiscard]] virtual size_t Size(void* apData) noexcept = 0;

  template <class T>
  [[nodiscard]] T* New() noexcept
  {
    static_assert(alignof(T) <= alignof(details::default_align_t));

    auto pData = static_cast<T*>(Allocate(sizeof(T)));
    if (pData) {
      return new (pData) T();
    }

    return nullptr;
  }

  template <class T, class... Args>
  [[nodiscard]] T* New(Args&&... args) noexcept
  {
    static_assert(alignof(T) <= alignof(details::default_align_t));

    auto pData = static_cast<T*>(Allocate(sizeof(T)));
    if (pData) {
      return new (pData) T(std::forward<Args>(args)...);
    }

    return nullptr;
  }

  template <class T>
  void Delete(T* apData) noexcept
  {
    if (apData == nullptr)
      return;

    apData->~T();
    Free(apData);
  }

  static void Push(AllocatorBase* apAllocator) noexcept;
  static void Push(AllocatorBase& aAllocator) noexcept;
  static AllocatorBase* Pop() noexcept;
  [[nodiscard]] static AllocatorBase* Get() noexcept;
  [[nodiscard]] static AllocatorBase* GetDefault() noexcept;
};

struct AllocatorCompatible
{
  TP_ALLOCATOR
};

struct ScopedAllocator
{
  explicit ScopedAllocator(AllocatorBase* apAllocator) noexcept;
  explicit ScopedAllocator(AllocatorBase& aAllocator) noexcept;
  ~ScopedAllocator() noexcept;

  TP_NOCOPYMOVE(ScopedAllocator);

private:
  AllocatorBase* m_pAllocator;
};

template <class T>
[[nodiscard]] T* New() noexcept
{
  if constexpr (details::has_allocator<T>)
    return AllocatorBase::Get()->New<T>();
  else
    return AllocatorBase::GetDefault()->New<T>();
}

template <class T, class... Args>
[[nodiscard]] T* New(Args&&... args) noexcept
{
  if constexpr (details::has_allocator<T>)
    return AllocatorBase::Get()->New<T>(std::forward<Args>(args)...);
  else
    return AllocatorBase::GetDefault()->New<T>(std::forward<Args>(args)...);
}

template <class T>
void Delete(T* apEntry) noexcept
{
  if constexpr (details::has_allocator<T>) {
    apEntry->GetAllocator()->Delete(apEntry);
  } else {
    AllocatorBase::GetDefault()->Delete(apEntry);
  }
}
}
