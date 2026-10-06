#include <catch2/catch_all.hpp>
#include <stdexcept>

// Enable the forbidden macro to test overflow behavior without checks
#define ROLLING_CONTAINER_DISABLE_STATIC_ASSERT
#include "RollingContainer.h"

using namespace Viet;

// Custom terminator that throws an exception instead of calling std::terminate
struct ThrowingTerminator
{
  [[noreturn]] static void Terminate()
  {
    throw std::runtime_error("RollingContainer bounds check failed");
  }
};

// Alias for checked container with throwing terminator
template <class T, size_t MaxLogicalIndex>
using CheckedContainer =
  RollingContainer<T, MaxLogicalIndex, BoundsChecks::Enabled,
                   ThrowingTerminator>;

// Alias for unchecked container with small MaxLogicalIndex (for testing
// overflow behavior)
template <class T, size_t MaxLogicalIndex>
using UncheckedContainer =
  RollingContainer<T, MaxLogicalIndex, BoundsChecks::Disabled>;

TEST_CASE("RollingContainer Operations", "[rolling_container]")
{
  SECTION("Basic Insertion and Access")
  {
    RollingContainer<int> container;
    REQUIRE(container.GetTotalProcessedCount() == 0);
    REQUIRE(container.GetActiveWindowStart() == 0);

    size_t offset0 = container.InsertBack(10);
    REQUIRE(offset0 == 0);
    size_t logicalIdx0 =
      (container.GetActiveWindowStart() + offset0) % container.SequenceModulus;
    REQUIRE(logicalIdx0 == 0);
    REQUIRE(container[0] == 10);
    REQUIRE(container.GetTotalProcessedCount() == 1);

    size_t offset1 = container.InsertBack(20);
    REQUIRE(offset1 == 1);
    size_t logicalIdx1 =
      (container.GetActiveWindowStart() + offset1) % container.SequenceModulus;
    REQUIRE(logicalIdx1 == 1);
    REQUIRE(container[1] == 20);
    REQUIRE(container.GetTotalProcessedCount() == 2);
  }

  SECTION("Rolling Window (ForgetAll)")
  {
    RollingContainer<int> container;
    container.InsertBack(1);
    container.InsertBack(2);
    container.InsertBack(3);

    REQUIRE(container.GetTotalProcessedCount() == 3);
    REQUIRE(container.GetActiveWindowStart() == 0);

    container.ForgetAll();

    REQUIRE(container.GetTotalProcessedCount() == 3);

    REQUIRE(container.GetActiveWindowStart() == 3);

    size_t offset = container.InsertBack(4);
    REQUIRE(offset == 0); // First element in new window

    // Logical index calculation:
    size_t logicalIdx =
      (container.GetActiveWindowStart() + offset) % container.SequenceModulus;
    REQUIRE(logicalIdx == 3); // 3 + 0

    REQUIRE(container[logicalIdx] == 4);
    REQUIRE(container.GetTotalProcessedCount() == 4);
  }

  SECTION("Circular Behavior with Small Modulus")
  {
    // MaxLogicalIndex = 4 => SequenceModulus = 5
    // Must use CheckedContainer here: static_assert requires checks for custom
    // MaxLogicalIndex
    CheckedContainer<int, 4> container;

    container.InsertBack(100); // logical 0
    container.InsertBack(101); // logical 1
    container.ForgetAll();
    // activeWindowStart = 2

    REQUIRE(container.GetActiveWindowStart() == 2);

    container.InsertBack(102); // logical 2
    container.InsertBack(103); // logical 3
    container.ForgetAll();
    // activeWindowStart = 2 + 2 = 4

    REQUIRE(container.GetActiveWindowStart() == 4);

    container.InsertBack(104); // logical 4

    REQUIRE(container[4] == 104);

    container.ForgetAll();
    // activeWindowStart = 4 + 1 = 5 => 0

    REQUIRE(container.GetActiveWindowStart() == 0);

    container.InsertBack(105); // logical 0 (wrapped around)
    REQUIRE(container[0] == 105);
  }

  SECTION("Iteration")
  {
    RollingContainer<int> container;
    container.InsertBack(10);
    container.InsertBack(20);
    container.InsertBack(30);

    int sum = 0;
    for (auto it = container.Begin(); it != container.End(); ++it) {
      sum += *it;
    }
    REQUIRE(sum == 60);

    container.ForgetAll();
    container.InsertBack(40);

    sum = 0;
    for (auto it = container.Begin(); it != container.End(); ++it) {
      sum += *it;
    }
    REQUIRE(sum == 40);
  }
}

TEST_CASE("RollingContainer Bounds Checks with Custom Terminator",
          "[rolling_container]")
{
  SECTION("InsertBack terminates on overflow")
  {
    CheckedContainer<int, 2> container; // SequenceModulus = 3

    container.InsertBack(10);
    container.InsertBack(20);
    container.InsertBack(30); // 3 elements, at the limit

    // 4th element exceeds SequenceModulus, should terminate
    REQUIRE_THROWS_AS(container.InsertBack(40), std::runtime_error);
  }

  SECTION("operator[] terminates on out-of-bounds access")
  {
    CheckedContainer<int, 4> container; // SequenceModulus = 5

    container.InsertBack(100);
    container.InsertBack(200);

    // Valid accesses
    REQUIRE(container[0] == 100);
    REQUIRE(container[1] == 200);

    // Out of bounds: only 2 elements, but trying to access logical index 2
    // offset = (2 + 5 - 0) % 5 = 2, but memoryWindow.size() == 2
    REQUIRE_THROWS_AS(container[2], std::runtime_error);
    REQUIRE_THROWS_AS(container[3], std::runtime_error);
    REQUIRE_THROWS_AS(container[4], std::runtime_error);
  }

  SECTION("const operator[] terminates on out-of-bounds access")
  {
    CheckedContainer<int, 4> container;
    container.InsertBack(100);

    const auto& constContainer = container;

    REQUIRE(constContainer[0] == 100);
    REQUIRE_THROWS_AS(constContainer[1], std::runtime_error);
  }

  SECTION("Out-of-bounds after ForgetAll")
  {
    CheckedContainer<int, 4> container;

    container.InsertBack(10);
    container.InsertBack(20);
    container.ForgetAll(); // activeWindowStart = 2, memoryWindow is empty

    // Now any access should fail since memoryWindow is empty
    REQUIRE_THROWS_AS(container[2], std::runtime_error);
    REQUIRE_THROWS_AS(container[0], std::runtime_error);

    // Insert new element
    container.InsertBack(30); // logical index 2

    // Valid access
    REQUIRE(container[2] == 30);

    // Invalid access (only 1 element in window)
    REQUIRE_THROWS_AS(container[3], std::runtime_error);
  }

  SECTION("Accessing old logical indices after ForgetAll")
  {
    CheckedContainer<int, 10> container;

    container.InsertBack(100); // logical 0
    container.InsertBack(200); // logical 1
    container.ForgetAll();     // activeWindowStart = 2

    container.InsertBack(300); // logical 2

    // Old indices 0 and 1 are now invalid
    // offset for 0 = (0 + 11 - 2) % 11 = 9, but memoryWindow.size() == 1
    REQUIRE_THROWS_AS(container[0], std::runtime_error);
    REQUIRE_THROWS_AS(container[1], std::runtime_error);

    // Current index is valid
    REQUIRE(container[2] == 300);
  }
}

TEST_CASE("RollingContainer Overflow Behavior (Unchecked)",
          "[rolling_container]")
{
  SECTION(
    "Unintended behavior when checks disabled with small MaxLogicalIndex")
  {
    // Using UncheckedContainer - only possible because we defined
    // ROLLING_CONTAINER_DISABLE_STATIC_ASSERT (the forbidden macro)
    // MaxLogicalIndex = 2 => SequenceModulus = 3
    UncheckedContainer<int, 2> container;

    // Insert more items than SequenceModulus without ForgetAll()
    container.InsertBack(10); // idx 0
    container.InsertBack(20); // idx 1
    container.InsertBack(30); // idx 2
    container.InsertBack(40); // idx 3 -> Overflow! vector index 3

    REQUIRE(container.GetTotalProcessedCount() == 4);

    // Iterator should still see everything because the vector just grew
    std::vector<int> values;
    for (auto it = container.Begin(); it != container.End(); ++it) {
      values.push_back(*it);
    }
    REQUIRE(values.size() == 4);
    REQUIRE(values == std::vector<int>{ 10, 20, 30, 40 });

    // But direct access via logical index is now broken/colliding due to
    // modulo arithmetic

    // Logical index 0: (0 + 3 - 0) % 3 = 0
    REQUIRE(container[0] == 10);

    // Logical index 3: (3 + 3 - 0) % 3 = 0
    // It should be 40, but implementation maps it to memoryWindow[0] which is
    // 10
    REQUIRE(container[3] == 10); // Unintended collision!

    // memoryWindow[3] (value 40) is unreachable via operator[] because
    // (idx + Mod - Start) % Mod is always < Mod (3)

    // Verify we can't get 40 via any logical index
    for (size_t i = 0; i < 10; ++i) {
      bool is40 = (container[i] == 40);
      REQUIRE_FALSE(is40);
    }
  }
}
