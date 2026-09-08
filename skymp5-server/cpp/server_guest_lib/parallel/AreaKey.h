#pragma once
#include <cstdint>
#include <functional>

namespace MpParallel {

// Side length of a grid chunk in Skyrim world units. Mirrors the divisor in
// MpObjectReference.cpp's GetGridPos; the two must agree or clusters would
// not line up with the subscription neighbourhoods they are meant to
// contain.
constexpr float kChunkSizeUnits = 4096.f;

// Smallest chunk separation that still guarantees two clusters cannot
// influence each other within one tick.
//
// Visibility is the 3x3 chunk stencil in Grid.h (reach 1), and movement
// validation caps one update at just under a chunk (reach 1 more), so an
// actor's relay set can span 2 chunks in a tick. 3 is the first value that
// keeps that entirely inside one cluster.
//
// Lives here rather than in ParallelConfig so the partitioner, which knows
// nothing about server settings, can enforce it.
constexpr int32_t kMinSafeSeparationChunks = 3;

[[nodiscard]] inline int16_t ToChunkCoord(float worldCoord) noexcept
{
  // Deliberately identical to GetGridPos: truncation towards zero, so
  // coordinates in (-4096, 4096) all map to chunk 0. Reproducing the quirk
  // matters more than fixing it, because the grid this partition mirrors
  // behaves the same way.
  return static_cast<int16_t>(worldCoord / kChunkSizeUnits);
}

// Identifies one chunk of one worldspace or interior cell.
struct AreaKey
{
  uint32_t worldOrCell = 0;
  int16_t chunkX = 0;
  int16_t chunkY = 0;

  [[nodiscard]] bool operator==(const AreaKey& rhs) const noexcept
  {
    return worldOrCell == rhs.worldOrCell && chunkX == rhs.chunkX &&
      chunkY == rhs.chunkY;
  }

  [[nodiscard]] bool operator!=(const AreaKey& rhs) const noexcept
  {
    return !(*this == rhs);
  }

  // Total order used to make partitioning deterministic across runs and
  // across worker counts.
  [[nodiscard]] bool operator<(const AreaKey& rhs) const noexcept
  {
    if (worldOrCell != rhs.worldOrCell) {
      return worldOrCell < rhs.worldOrCell;
    }
    if (chunkX != rhs.chunkX) {
      return chunkX < rhs.chunkX;
    }
    return chunkY < rhs.chunkY;
  }

  // Chebyshev distance in chunks. Returns -1 when the two keys live in
  // different worldspaces, which callers treat as "infinitely far apart".
  [[nodiscard]] int32_t ChunkDistanceTo(const AreaKey& rhs) const noexcept
  {
    if (worldOrCell != rhs.worldOrCell) {
      return -1;
    }
    const int32_t dx =
      static_cast<int32_t>(chunkX) - static_cast<int32_t>(rhs.chunkX);
    const int32_t dy =
      static_cast<int32_t>(chunkY) - static_cast<int32_t>(rhs.chunkY);
    const int32_t absDx = dx < 0 ? -dx : dx;
    const int32_t absDy = dy < 0 ? -dy : dy;
    return absDx > absDy ? absDx : absDy;
  }
};

struct AreaKeyHash
{
  [[nodiscard]] size_t operator()(const AreaKey& key) const noexcept
  {
    // Pack into 64 bits first: the three fields together are exactly 64 bits
    // wide, so no information is lost and the mix below sees every input bit.
    uint64_t packed = static_cast<uint64_t>(key.worldOrCell) << 32;
    packed |= static_cast<uint64_t>(static_cast<uint16_t>(key.chunkX)) << 16;
    packed |= static_cast<uint64_t>(static_cast<uint16_t>(key.chunkY));

    // splitmix64 finalizer.
    packed ^= packed >> 30;
    packed *= 0xbf58476d1ce4e5b9ULL;
    packed ^= packed >> 27;
    packed *= 0x94d049bb133111ebULL;
    packed ^= packed >> 31;
    return static_cast<size_t>(packed);
  }
};

}
