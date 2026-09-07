#include "libespm/Combiner.h"
#include "libespm/Browser.h"
#include "libespm/Convert.h"
#include "libespm/Records.h"
#include "libespm/Utils.h"
#include "libespm/espm.h"
#include <array>
#include <fmt/format.h>
#include <string>
#include <vector>

namespace espm {

Combiner::Combiner()
  : pImpl(nullptr)
{
  pImpl = std::make_unique<CombineBrowser::Impl>();
}

void espm::Combiner::AddSource(Browser* src, const char* fileName) noexcept
{
  if (pImpl->numSources >= std::size(pImpl->sources)) {
    ++pImpl->numSources;
    return;
  }
  pImpl->sources[pImpl->numSources++] = { src, fileName, nullptr };
}

namespace {
// TES4 header flag 0x200 (CommonLibSSE RecordFlag::kSmallFile) marks a light
// plugin. Light plugins are indexed in the 0xFE space by the runtime, so the
// server has to match or every form id past the first ESL resolves wrong.
constexpr uint32_t kSmallFileFlag = 0x200;
}

std::unique_ptr<espm::CombineBrowser> Combiner::Combine()
{
  if (pImpl->numSources > std::size(pImpl->sources)) {
    throw CombineError("too many sources");
  }

  // Pass 1: classify every source and assign its slot in the combined space.
  // Full and light plugins are counted separately, exactly like the game does.
  std::vector<PluginSlot> slots(pImpl->numSources);
  uint32_t nextFull = 0, nextLight = 0;
  for (size_t i = 0; i < pImpl->numSources; ++i) {
    auto& src = pImpl->sources[i];
    if (!src.br) {
      throw CombineError("nullptr source with index " + std::to_string(i));
    }
    const auto tes4Rec = src.br->LookupById(0);
    if (!Convert<TES4>(tes4Rec)) {
      throw CombineError(src.fileName + " doesn't have TES4 record");
    }
    const bool light = (tes4Rec->GetFlags() & kSmallFileFlag) != 0;
    if (light) {
      if (nextLight > 0xFFF) {
        throw CombineError("too many light plugins (max 4096)");
      }
      slots[i] = PluginSlot{ true, nextLight++ };
    } else {
      if (nextFull > 0xFD) {
        throw CombineError("too many full plugins (max 254)");
      }
      slots[i] = PluginSlot{ false, nextFull++ };
    }
  }

  // Pass 2: build the per-source mappings using those slots.
  for (size_t i = 0; i < pImpl->numSources; ++i) {
    auto& src = pImpl->sources[i];

    const auto tes4 = Convert<TES4>(src.br->LookupById(0));
    espm::CompressedFieldsCache dummyCache;
    const auto masters = tes4->GetData(dummyCache).masters;

    auto toComb = std::make_unique<IdMapping>();
    auto toRaw = std::make_unique<IdMapping>();

    // Inside a plugin file, a reference's high byte is always an index into
    // that file's own master list, even when the master is light. Only the
    // combined side uses the 0xFE space.
    size_t m = 0;
    for (m = 0; m < masters.size(); ++m) {
      const int globalIdx = pImpl->GetFileIndex(masters[m]);
      if (globalIdx == -1) {
        throw CombineError(src.fileName + " has unresolved dependency (" +
                           masters[m] + ")");
      }
      const PluginSlot rawSlot{ false, static_cast<uint32_t>(m) };
      toComb->Set(rawSlot, slots[globalIdx]);
      toRaw->Set(slots[globalIdx], rawSlot);
    }
    // The file's own records use the slot right after its masters.
    const PluginSlot ownRaw{ false, static_cast<uint32_t>(m) };
    toComb->Set(ownRaw, slots[i]);
    toRaw->Set(slots[i], ownRaw);

    src.toComb = std::move(toComb);
    src.toRaw = std::move(toRaw);
    src.slot = slots[i];
  }

  std::unique_ptr<espm::CombineBrowser> res(new CombineBrowser);
  res->pImpl = pImpl;
  return res;
}

Combiner::~Combiner() = default;

}
