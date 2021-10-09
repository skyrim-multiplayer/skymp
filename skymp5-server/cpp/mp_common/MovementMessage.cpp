#include "MovementMessage.h"

namespace {
const inline std::string kStanding = "Standing";
const inline std::string kWalking = "Walking";
const inline std::string kRunning = "Running";
const inline std::string kSprinting = "Sprinting";
}

const std::string& ToString(RunMode runMode)
{
  switch (runMode) {
    case RunMode::Standing:
      return kStanding;
    case RunMode::Walking:
      return kWalking;
    case RunMode::Running:
      return kRunning;
    case RunMode::Sprinting:
      return kSprinting;
    default:
      throw std::runtime_error("unhandled case for RunMode");
  }
}

RunMode RunModeFromString(std::string_view str)
{
  if (str == kStanding) {
    return RunMode::Standing;
  } else if (str == kWalking) {
    return RunMode::Walking;
  } else if (str == kRunning) {
    return RunMode::Running;
  } else if (str == kSprinting) {
    return RunMode::Sprinting;
  } else {
    throw std::runtime_error("cannot parse RunMode from " +
                             std::string{ str });
  }
}
