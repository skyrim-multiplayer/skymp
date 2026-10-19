#include "AppearanceValidation.h"
#include "WorldState.h"
#include "libespm/RACE.h"
#include <fmt/format.h>

namespace AppearanceValidation {

std::string Validate(WorldState& worldState, const Appearance& appearance)
{
  // Validate character name
  if (appearance.name.empty()) {
    return "Character name must not be empty";
  }

  constexpr size_t kMaxNameLength = 64;
  if (appearance.name.size() > kMaxNameLength) {
    return fmt::format("Character name exceeds maximum length ({} > {})",
                       appearance.name.size(), kMaxNameLength);
  }

  for (char c : appearance.name) {
    if (static_cast<unsigned char>(c) < 0x20) {
      return "Character name contains invalid control characters";
    }
  }

  // Validate raceId against espm (skipped gracefully if espm is not loaded)
  if (!worldState.HasEspm()) {
    return "";
  }

  auto& br = worldState.GetEspm().GetBrowser();
  auto& cache = worldState.GetEspmCache();

  auto raceLookup = br.LookupById(appearance.raceId);
  if (!raceLookup.rec) {
    return fmt::format("Race {:#x} not found in espm", appearance.raceId);
  }

  auto* race = espm::Convert<espm::RACE>(raceLookup.rec);
  if (!race) {
    return fmt::format("Form {:#x} is not a RACE record", appearance.raceId);
  }

  auto raceData = race->GetData(cache);
  if (!(raceData.flags & espm::RACE::kPlayable)) {
    return fmt::format("Race {:#x} is not a playable race", appearance.raceId);
  }

  return "";
}

} // namespace AppearanceValidation
