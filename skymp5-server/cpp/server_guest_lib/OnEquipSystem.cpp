#include "OnEquipSystem.h"

#include "EspmGameObject.h"
#include "MpActor.h"
#include "MpObjectReference.h"
#include "Structures.h"
#include "SweetpieScript.h"
#include "WorldState.h"
#include "espm.h"

void OnEquipSystem::Run(WorldState& worldState, uint32_t formId,
                        uint32_t itemBaseId)
{
  auto [actor, objectReference] =
    worldState.Get<MpActor, MpObjectReference>(formId);

  if (objectReference.GetInventory().GetItemCount(itemBaseId) == 0)
    return;

  espm::Loader& espm = worldState.GetEspm();
  const espm::LookupResult result = espm.GetBrowser().LookupById(itemBaseId);

  if (!result.rec)
    return;

  const espm::Type type = result.rec->GetType();

  const std::unordered_set<std::string> s = { worldState.espmFiles.begin(),
                                              worldState.espmFiles.end() };
  const bool hasSweetpie = s.count("SweetPie.esp");

  if (type == "INGR" || type == "ALCH") {
    std::vector<espm::Effects::Effect> effects;
    if (type == "ALCH") {
      effects = espm::GetData<espm::ALCH>(itemBaseId, &espm).effects;
    } else if (type == "INGR") {
      effects = espm::GetData<espm::INGR>(itemBaseId, &espm).effects;
    } else {
      return;
    }

    for (const auto& effect : effects) {
      const espm::ActorValue av =
        espm::GetData<espm::MGEF>(effect.effectId, &espm).data.primaryAV;
      if (av == espm::ActorValue::Health || av == espm::ActorValue::Stamina ||
          av == espm::ActorValue::Magicka) { // other types are unsupported
        if (hasSweetpie) {
          if (actor.CanActorValueBeRestored(av)) {
            // this coefficient (workaround) has been added for sake of game
            // balance and because of disability to restrict players use
            // potions often on client side
            constexpr float kMagnitudeCoeff = 100.f;
            actor.RestoreActorValue(av, effect.magnitude * kMagnitudeCoeff);
          }
        } else {
          actor.RestoreActorValue(av, effect.magnitude);
        }
      }
    }

    objectReference.RemoveItem(itemBaseId, 1, nullptr);
    const VarValue args[] = {
      VarValue{ std::make_shared<EspmGameObject>(result) }, VarValue::None()
    };
    objectReference.SendPapyrusEvent("OnObjectEquipped", args,
                                     std::size(args));
  }

  if (hasSweetpie) {
    SweetPieScript SweetPieScript(worldState.espmFiles);
    SweetPieScript.Play(actor, worldState, itemBaseId);
  }
}