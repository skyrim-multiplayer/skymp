#pragma once
#include <array>
#include <cstdint>
#include <unordered_map>

namespace espm {

// Where a plugin sits in a form-id space. Skyrim has two: full plugins get a
// byte-wide index plus 24 bits of local id, light plugins (ESL / ESPFE) live
// under the 0xFE prefix with a 12-bit slot and 12 bits of local id.
// Runtime formula (CommonLibSSE TESDataHandler::LookupFormID):
//   compileIndex << 24 | smallFileCompileIndex << 12 | localFormId
struct PluginSlot
{
  bool light = false;
  uint32_t index = 0; // 0..0xFD when full, 0..0xFFF when light

  uint32_t Base() const noexcept
  {
    return light ? (0xFE000000u | ((index & 0xFFFu) << 12))
                 : ((index & 0xFFu) << 24);
  }

  uint32_t LocalMask() const noexcept { return light ? 0xFFFu : 0xFFFFFFu; }
};

// Translates form ids between spaces (raw per-file space <-> combined space).
// Full source indices use the array fast path; light sources are keyed by their
// 12-bit slot because they all share the 0xFE high byte.
class IdMapping
{
public:
  // Out of the valid id range, so callers keep using the existing
  // ">= 0xff000000 means skip this source" contract.
  static constexpr uint32_t kInvalid = 0xFFFFFFFFu;

  IdMapping() { mapped.fill(false); }

  void Set(const PluginSlot& from, const PluginSlot& to) noexcept
  {
    if (from.light) {
      light[static_cast<uint16_t>(from.index & 0xFFFu)] = to;
      return;
    }
    const uint32_t i = from.index & 0xFFu;
    full[i] = to;
    mapped[i] = true;
  }

  uint32_t Map(uint32_t id) const noexcept
  {
    const uint32_t high = id >> 24;
    if (high == 0xFEu) {
      const auto it = light.find(static_cast<uint16_t>((id >> 12) & 0xFFFu));
      if (it == light.end()) {
        return kInvalid;
      }
      return it->second.Base() | (id & 0xFFFu);
    }
    if (!mapped[high]) {
      return kInvalid;
    }
    const PluginSlot& to = full[high];
    return to.Base() | (id & to.LocalMask());
  }

private:
  std::array<PluginSlot, 256> full{};
  std::array<bool, 256> mapped{};
  std::unordered_map<uint16_t, PluginSlot> light;
};

}
