#include "Allocator.hpp"
#include <cassert>

namespace CEFUtils {
struct AllocatorStack
{
  enum
  {
    kMaxAllocatorCount = 1024
  };

  AllocatorStack() noexcept
    : m_index(0)
  {
    m_stack[0] = AllocatorBase::GetDefault();
  }

  void Push(AllocatorBase* apAllocator) noexcept
  {
    assert(m_index + 1 < kMaxAllocatorCount);

    m_index++;
    m_stack[m_index] = apAllocator;
  }

  AllocatorBase* Pop() noexcept
  {
    assert(m_index > 0);

    const auto pAllocator = m_stack[m_index];
    m_index--;

    return pAllocator;
  }

  [[nodiscard]] AllocatorBase* Get() noexcept { return m_stack[m_index]; }

  [[nodiscard]] static AllocatorStack& Instance() noexcept
  {
    static thread_local AllocatorStack s_stack;
    return s_stack;
  }

private:
  uint32_t m_index;
  AllocatorBase* m_stack[kMaxAllocatorCount]{};
};

void AllocatorBase::Push(AllocatorBase* apAllocator) noexcept
{
  AllocatorStack::Instance().Push(apAllocator);
}

void AllocatorBase::Push(AllocatorBase& aAllocator) noexcept
{
  Push(&aAllocator);
}

AllocatorBase* AllocatorBase::Pop() noexcept
{
  return AllocatorStack::Instance().Pop();
}

AllocatorBase* AllocatorBase::Get() noexcept
{
  return AllocatorStack::Instance().Get();
}

AllocatorBase* AllocatorBase::GetDefault() noexcept
{
  class DefaultAllocator : public AllocatorBase
  {
  public:
    [[nodiscard]] void* Allocate(size_t aSize) noexcept override
    {
      return malloc(aSize);
    }

    void Free(void* apData) noexcept override { return free(apData); }

    [[nodiscard]] size_t Size(void* apData) noexcept override
    {
      return _msize(apData);
    }
  };

  static DefaultAllocator s_allocator;
  return &s_allocator;
}

ScopedAllocator::ScopedAllocator(AllocatorBase* apAllocator) noexcept
  : m_pAllocator(apAllocator)
{
  AllocatorBase::Push(m_pAllocator);
}

ScopedAllocator::ScopedAllocator(AllocatorBase& aAllocator) noexcept
  : ScopedAllocator(&aAllocator)
{
}

ScopedAllocator::~ScopedAllocator() noexcept
{
  AllocatorBase::Pop();
}

}
