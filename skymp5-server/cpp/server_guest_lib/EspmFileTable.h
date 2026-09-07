#pragma once
#include "libespm/IdMapping.h"
#include <string>
#include <vector>

// The loaded plugin list plus each plugin's slot in the form-id space.
// Derives from vector<string> so the many places that only need the names keep
// working unchanged; FormDesc uses the slots to encode light (ESL) plugins into
// the 0xFE space the game runtime uses.
class EspmFileTable : public std::vector<std::string>
{
public:
  using std::vector<std::string>::vector;

  EspmFileTable() = default;

  // Inherited constructors do not include the base copy/move ones, so allow
  // assigning a plain name list (espm::Loader::GetFileNames) directly.
  EspmFileTable(std::vector<std::string> names)
    : std::vector<std::string>(std::move(names))
  {
  }

  EspmFileTable& operator=(std::vector<std::string> names)
  {
    std::vector<std::string>::operator=(std::move(names));
    slots.clear();
    return *this;
  }

  // Without explicit slots every file is treated as a full plugin indexed by
  // position, which is exactly the pre-ESL behaviour (used by tests).
  espm::PluginSlot SlotAt(size_t i) const
  {
    if (i < slots.size()) {
      return slots[i];
    }
    return espm::PluginSlot{ false, static_cast<uint32_t>(i) };
  }

  void SetSlots(std::vector<espm::PluginSlot> slots_)
  {
    slots = std::move(slots_);
  }

  // Index of the file owning this form id, or -1. Handles both spaces.
  int FileIndexOf(uint32_t formId) const
  {
    const bool light = (formId >> 24) == 0xFEu;
    const uint32_t idx =
      light ? ((formId >> 12) & 0xFFFu) : ((formId >> 24) & 0xFFu);
    const int n = static_cast<int>(size());
    for (int i = 0; i < n; ++i) {
      const espm::PluginSlot slot = SlotAt(i);
      if (slot.light == light && slot.index == idx) {
        return i;
      }
    }
    return -1;
  }

private:
  std::vector<espm::PluginSlot> slots;
};
