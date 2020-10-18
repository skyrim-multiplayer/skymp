#include "PapyrusDebug.h"

#include "NetworkingInterface.h" // Format
#include "SpSnippet.h"
#include "WorldState.h"
#include <nlohmann/json.hpp>
#include <sstream>

VarValue PapyrusDebug::Notification(VarValue self,
                                    const std::vector<VarValue>& arguments)
{
  if (arguments.size() >= 1 &&
      arguments[0].GetType() == VarValue::kType_String) {

    auto str = static_cast<const char*>(arguments[0]);
    auto len = strlen(str);

    std::stringstream ss;
    ss << '[' << nlohmann::json(str).dump() << ']';
    auto s = ss.str();

    if (auto actor = compatibilityPolicy->GetDefaultActor())
      SpSnippet("Debug", "Notification", s.data()).Send(actor);
  }

  return VarValue::None();
}