#include "libespm/Combiner.h"
#include "libespm/Browser.h"
#include "libespm/Convert.h"
#include "libespm/Records.h"
#include "libespm/Utils.h"
#include "libespm/espm.h"
#include <array>
#include <fmt/format.h>
#include <string>

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

std::unique_ptr<espm::CombineBrowser> Combiner::Combine()
{
  if (pImpl->numSources > std::size(pImpl->sources)) {
    throw CombineError("too many sources");
  }

  for (size_t i = 0; i < pImpl->numSources; ++i) {
    auto& src = pImpl->sources[i];
    if (!src.br) {
      throw CombineError("nullptr source with index " + std::to_string(i));
    }

    const auto tes4 = Convert<TES4>(src.br->LookupById(0));
    if (!tes4) {
      throw CombineError(src.fileName + " doesn't have TES4 record");
    }
    espm::CompressedFieldsCache dummyCache;
    const auto masters = tes4->GetData(dummyCache).masters;

    auto toComb = std::make_unique<IdMapping>();
    toComb->fill(0xff);
    auto toRaw = std::make_unique<IdMapping>();
    toRaw->fill(0xff);
    size_t m = 0;
    for (m = 0; m < masters.size(); ++m) {
      const int globalIdx = pImpl->GetFileIndex(masters[m]);
      if (globalIdx == -1) {
        throw CombineError(src.fileName + " has unresolved dependency (" +
                           masters[m] + ")");
      }
      (*toComb)[m] = static_cast<uint8_t>(globalIdx);
      (*toRaw)[globalIdx] = static_cast<uint8_t>(m);
    }
    (*toComb)[m] = static_cast<uint8_t>(i);
    (*toRaw)[i] = static_cast<uint8_t>(m);
    src.toComb = std::move(toComb);
    src.toRaw = std::move(toRaw);
  }

  std::unique_ptr<espm::CombineBrowser> res(new CombineBrowser);
  res->pImpl = pImpl;
  return res;
}

Combiner::~Combiner() = default;

}
