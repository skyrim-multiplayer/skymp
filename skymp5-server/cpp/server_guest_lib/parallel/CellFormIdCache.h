#pragma once
#include "FormDesc.h"
#include <cstdint>
#include <string>
#include <vector>

namespace MpParallel {

// One-entry memo over FormDesc::ToFormId.
//
// ToFormId resolves a plugin name by walking the load order and comparing
// std::strings until it matches, so a single call is O(plugins) string
// comparisons. The offload path needs one per connected player per tick to
// build the relay-target snapshot, plus one per movement packet, and on a
// server with a real load order that is tens of thousands of string
// comparisons a tick on the thread that can least afford them.
//
// In practice every one of those calls names the same worldspace -- everybody
// is in Tamriel -- so remembering the last answer turns the scan into one
// comparison. A miss simply pays what the call cost before.
//
// Not thread-safe, and not meant to be: it is main-thread bookkeeping done
// during ingest and tick, never inside the parallel phase.
class CellFormIdCache
{
public:
  // Throws whatever FormDesc::ToFormId throws, i.e. std::runtime_error when
  // the plugin is not in the load order. A cached entry never throws.
  [[nodiscard]] uint32_t Resolve(const FormDesc& desc,
                                 const std::vector<std::string>& files)
  {
    if (valid && desc.shortFormId == cachedShortFormId &&
        &files == cachedFiles && files.size() == cachedFileCount &&
        desc.file == cachedFile) {
      return cachedFormId;
    }

    const uint32_t formId = desc.ToFormId(files);

    cachedShortFormId = desc.shortFormId;
    cachedFile = desc.file;
    cachedFormId = formId;
    // The load order is fixed once the server is up, but keying on the
    // container identity and size means a test or an embedder that swaps it
    // out cannot be served a stale answer.
    cachedFiles = &files;
    cachedFileCount = files.size();
    valid = true;
    return formId;
  }

  void Invalidate() noexcept { valid = false; }

private:
  bool valid = false;
  uint32_t cachedShortFormId = 0;
  std::string cachedFile;
  uint32_t cachedFormId = 0;
  const std::vector<std::string>* cachedFiles = nullptr;
  size_t cachedFileCount = 0;
};

}
