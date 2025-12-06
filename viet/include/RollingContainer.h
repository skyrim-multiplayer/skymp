#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>

namespace Viet
{
template <class T, size_t MaxLogicalIndex = 0x1FFFFFFFFFFFFF>
class RollingContainer
{
public:
  static constexpr size_t SequenceModulus = MaxLogicalIndex + 1;

  using Iterator = T*;

  Iterator Begin() { return memoryWindow.data(); }
  Iterator End() { return memoryWindow.data() + memoryWindow.size(); }

  T& operator[](size_t logicalIndex)
  {
    size_t offset =
      (logicalIndex + SequenceModulus - activeWindowStart) % SequenceModulus;

    // NO CHECKS to save CPU cycles.
    // We assume 'offset' is within [0, memoryWindow.size())
    return memoryWindow[offset];
  }

  const T& operator[](size_t logicalIndex) const
  {
    size_t offset =
      (logicalIndex + SequenceModulus - activeWindowStart) % SequenceModulus;
    return memoryWindow[offset];
  }

  size_t GetTotalProcessedCount() const
  {
    return memoryWindow.size() + activeWindowStart;
  }

  size_t GetActiveWindowStart() const { return activeWindowStart; }

  size_t InsertBack(const T& value)
  {
    // SAFETY CHECK OMITTED FOR PERFORMANCE
    // Theoretical Panic: if (memoryWindow.size() > MaxLogicalIndex)
    // std::terminate(); Reason: If vector grows larger than the logical ring,
    // the head overwrites the tail.

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
