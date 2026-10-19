#pragma once
#include "Appearance.h"
#include <string>

class WorldState;

namespace AppearanceValidation {

// Returns an empty string if the appearance is valid.
// Returns an error message if validation fails.
// Gracefully skips espm-based checks when no espm is loaded.
std::string Validate(WorldState& worldState, const Appearance& appearance);

}
