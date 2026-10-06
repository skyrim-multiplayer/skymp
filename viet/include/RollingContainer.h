#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Viet {

enum class BoundsChecks
{
  Disabled,
  Enabled
};

struct DefaultTerminator
{
  [[noreturn]] static void Terminate() { std::terminate(); }
};

// The default MaxLogicalIndex is astronomically large (2^53 - 1). At this
// scale, you'd need mass planetary storage to overflow the container, making
// runtime bounds checks pure overhead. Disabling them is safe and saves CPU
// cycles.
//
// However, if you shrink MaxLogicalIndex (e.g., for testing or constrained
// use), overflow becomes realistic. In that case, we enforce
// BoundsChecks::Enabled via static_assert to catch misuse at compile time.
//
// Define ROLLING_CONTAINER_DISABLE_STATIC_ASSERT before including this header
// to bypass the safety check. This is for testing purposes ONLY. If you're
// tempted to use this in production, reconsider your life choices.
inline constexpr size_t kDefaultMaxLogicalIndex = 0x1FFFFFFFFFFFFF;

template <class T, size_t MaxLogicalIndex = kDefaultMaxLogicalIndex,
          BoundsChecks Checks = BoundsChecks::Disabled,
          class Terminator = DefaultTerminator>
class RollingContainer
{
#ifndef ROLLING_CONTAINER_DISABLE_STATIC_ASSERT
  static_assert(MaxLogicalIndex == kDefaultMaxLogicalIndex ||
                  Checks == BoundsChecks::Enabled,
                "BoundsChecks must be Enabled when using a custom (smaller) "
                "MaxLogicalIndex. Disabling checks is only safe with the "
                "default MaxLogicalIndex, which is large enough that overflow "
                "is practically impossible.");
#endif

public:
  static constexpr size_t SequenceModulus = MaxLogicalIndex + 1;

  using Iterator = T*;

  Iterator Begin() { return memoryWindow.data(); }
  Iterator End() { return memoryWindow.data() + memoryWindow.size(); }

  T& operator[](size_t logicalIndex)
  {
    size_t offset =
      (logicalIndex + SequenceModulus - activeWindowStart) % SequenceModulus;

    if constexpr (Checks == BoundsChecks::Enabled) {
      if (offset >= memoryWindow.size()) {
        Terminator::Terminate();
      }
    }

    return memoryWindow[offset];
  }

  const T& operator[](size_t logicalIndex) const
  {
    size_t offset =
      (logicalIndex + SequenceModulus - activeWindowStart) % SequenceModulus;

    if constexpr (Checks == BoundsChecks::Enabled) {
      if (offset >= memoryWindow.size()) {
        Terminator::Terminate();
      }
    }

    return memoryWindow[offset];
  }

  size_t GetTotalProcessedCount() const
  {
    return memoryWindow.size() + activeWindowStart;
  }

  size_t GetActiveWindowStart() const { return activeWindowStart; }

  size_t InsertBack(const T& value)
  {
    if constexpr (Checks == BoundsChecks::Enabled) {
      if (memoryWindow.size() >= SequenceModulus) {
        Terminator::Terminate();
      }
    }

    memoryWindow.push_back(value);
    return memoryWindow.size() - 1;
  }

  void ForgetAll()
  {
    // Advance the start pointer by the number of elements we processed.
    // We must modulo here to ensure the start index wraps correctly upon
    // reaching Max.
    activeWindowStart =
      (activeWindowStart + memoryWindow.size()) % SequenceModulus;
    memoryWindow.clear();
  }

private:
  std::vector<T> memoryWindow;
  size_t activeWindowStart = 0;
};
}
