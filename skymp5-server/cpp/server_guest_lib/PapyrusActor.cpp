#include "PapyrusActor.h"

#include "MpActor.h"
#include "MpFormGameObject.h"

namespace {
std::string ToLowerCase(std::string str)
{
  for (auto& ch : str) {
    ch = std::tolower(ch);
  }
  return str;
}

espm::ActorValue ConvertToAV(const char* actorValueName)
{
  std::string name = ToLowerCase(actorValueName);
  if (name == "health") {
    return espm::ActorValue::Health;
  }
  if (name == "stamina") {
    return espm::ActorValue::Stamina;
  }
  if (name == "magicka") {
    return espm::ActorValue::Magicka;
  }
  return espm::ActorValue::None;
}

#define ACTOR_VALUE_CHANGE(functionName)                                      \
  espm::ActorValue attributeName =                                            \
    ConvertToAV(static_cast<const char*>(arguments[0]));                      \
  float modifire = static_cast<double>(arguments[1]);                         \
  if (auto actor = GetFormPtr<MpActor>(self)) {                               \
    actor->functionName(attributeName, modifire);                             \
  }                                                                           \
  return VarValue();
}

VarValue PapyrusActor::IsWeaponDrawn(VarValue self,
                                     const std::vector<VarValue>& arguments)
{
  if (auto actor = GetFormPtr<MpActor>(self)) {
    return VarValue(actor->IsWeaponDrawn());
  }
  return VarValue(false);
}

VarValue PapyrusActor::RestoreActorValue(
  VarValue self, const std::vector<VarValue>& arguments){
  ACTOR_VALUE_CHANGE(RestoreActorValue)
}

VarValue PapyrusActor::DamageActorValue(VarValue self,
                                        const std::vector<VarValue>& arguments)
{
  ACTOR_VALUE_CHANGE(DamageActorValue)
}
