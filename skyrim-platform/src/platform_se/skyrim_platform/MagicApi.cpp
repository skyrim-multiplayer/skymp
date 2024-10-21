#include "MagicApi.h"
#include "CallNativeApi.h"
#include "JsUtils.h"
#include "SkyrimPlatform.h"

#include "Magic/AnimationGraphMasterBehaviourDescriptor.h"

extern CallNativeApi::NativeCallRequirements g_nativeCallRequirements;

namespace skymp::magic::details {

AnimationGraphMasterBehaviourDescriptor::AnimationVariables
GetAnimationVariablesFromJSArg(const JsValue& argObj)
{
  using AnimVarInitializer =
    AnimationGraphMasterBehaviourDescriptor::AnimationVariables::InitData;

  const auto booleanVarsValue = argObj.GetProperty("booleans");

  const auto booleanVars = AnimVarInitializer{
    static_cast<uint8_t*>(booleanVarsValue.GetTypedArrayData()),
    booleanVarsValue.GetTypedArrayBufferLength()
  };

  const auto floatsVarsValue = argObj.GetProperty("floats");

  const auto floatsVars = AnimVarInitializer{
    static_cast<uint8_t*>(floatsVarsValue.GetTypedArrayData()),
    floatsVarsValue.GetTypedArrayBufferLength()
  };

  const auto integersVarsValue = argObj.GetProperty("integers");

  const auto integersVars = AnimVarInitializer{
    static_cast<uint8_t*>(integersVarsValue.GetTypedArrayData()),
    integersVarsValue.GetTypedArrayBufferLength()
  };

  const auto variables =
    AnimationGraphMasterBehaviourDescriptor::AnimationVariables{
      booleanVars, floatsVars, integersVars
    };

  return variables;
}

} // namespace skymp::magic::details

JsValue MagicApi::CastSpellImmediate(const JsFunctionArguments& args)
{
  const auto actorFormId =
    static_cast<RE::FormID>(static_cast<double>(args[1]));

  const auto castingSource =
    static_cast<RE::MagicSystem::CastingSource>(static_cast<int>(args[2]));

  const auto spellFormId =
    static_cast<RE::FormID>(static_cast<double>(args[3]));

  const auto magicTarget = RE::TESForm::LookupByID<RE::TESObjectREFR>(
    static_cast<uint32_t>(static_cast<double>(args[4])));

  const auto aimAngle = static_cast<float>(static_cast<double>(args[5]));
  const auto aimHeading = static_cast<float>(static_cast<double>(args[6]));
  const RE::Projectile::ProjectileRot projectileAngles{ aimAngle, aimHeading };

  g_nativeCallRequirements.gameThrQ->AddTask(
    [spellFormId, actorFormId, castingSource, magicTarget, projectileAngles,

     animVars =
       skymp::magic::details::GetAnimationVariablesFromJSArg(args[7])]() {
      auto* pSpell = RE::TESForm::LookupByID<RE::SpellItem>(spellFormId);

      auto* pActor = RE::TESForm::LookupByID<RE::Actor>(actorFormId);

      if (!pSpell || !pActor) {
        return;
      }

      const auto t = pSpell->GetFormType();

      const bool isValidSpellType = t == RE::FormType::Spell ||
        t == RE::FormType::Scroll || t == RE::FormType::Ingredient ||
        t == RE::FormType::AlchemyItem || t == RE::FormType::Enchantment;

      if (!isValidSpellType) {
        return;
      }

      const bool isAnimationVariablesApplied =
        AnimationGraphMasterBehaviourDescriptor{ std::move(animVars) }
          .ApplyVariablesToActor(*pActor);

      if (!isAnimationVariablesApplied) {
        return;
      }

      const auto magicCaster = pActor->GetMagicCaster(castingSource);

      if (!magicCaster) {
        return;
      }

      if (pSpell->GetCastingType() ==
          RE::MagicSystem::CastingType::kConcentration) {

        magicCaster->CastSpellImmediate(pSpell, false, magicTarget, 1.0f,
                                        false, 0.0f, pActor);

        return;
      }

      RE::ProjectileHandle pProjectile{};

      const auto magicNode = magicCaster->GetMagicNode();

      RE::NiPoint3 origin =
        magicNode ? magicNode->world.translate : pActor->GetPosition();

      if (!magicNode) {
        const auto boundMax = pActor->GetBoundMax();
        const auto boundMin = pActor->GetBoundMin();
        origin.z += (boundMax.z - boundMin.z) * 0.7f;
      }

      if (pSpell->data.delivery ==
          RE::MagicSystem::Delivery::kTargetLocation) {
        // TODO we need recalculate origin, cast ray from head to crosshair
        // direction

        // Maybe anything of this?
        // pActor->GetLookingAtLocation();
        // Or
        // auto* crosshairPickData = RE::CrosshairPickData::GetSingleton();
        // Or
        /*const auto hud =
          RE::UI::GetSingleton()->GetMenu<RE::HUDMenu>(RE::HUDMenu::MENU_NAME);

        hud->UpdateCrosshairMagicTarget(true);*/
        // Or
        auto viewDirection = pActor->Get3D2()->world.rotate.GetVectorY();
        viewDirection.Unitize();
        origin += viewDirection * 200.f;
        origin.z = pActor->GetPositionZ();
      }

      RE::Projectile::LaunchData launchData(pActor, origin, projectileAngles,
                                            pSpell);

      launchData.castingSource = castingSource;
      launchData.desiredTarget = magicTarget;
      launchData.contactNormal = RE::NiPoint3{ 0.f, 0.f, 1.0f };

      RE::Projectile::Launch(&pProjectile, launchData);
    });

  return JsValue::Undefined();
}

JsValue MagicApi::InterruptCast(const JsFunctionArguments& args)
{
  const auto actorFormId =
    static_cast<RE::FormID>(static_cast<double>(args[1]));

  const auto castingSource =
    static_cast<RE::MagicSystem::CastingSource>(static_cast<int>(args[2]));

  g_nativeCallRequirements.gameThrQ->AddTask(
    [actorFormId, castingSource,
     animVars =
       skymp::magic::details::GetAnimationVariablesFromJSArg(args[3])]() {
      const auto pActor = RE::TESForm::LookupByID<RE::Actor>(actorFormId);
      if (!pActor) {
        return;
      }

      const bool isAnimationVariablesApplied =
        AnimationGraphMasterBehaviourDescriptor{ std::move(animVars) }
          .ApplyVariablesToActor(*pActor);

      if (!isAnimationVariablesApplied) {
        return;
      }

      if (auto* caster = pActor->GetMagicCaster(castingSource)) {
        caster->FinishCast();
      } else {
        pActor->InterruptCast(false);
      }
    });

  return JsValue::Undefined();
}

JsValue MagicApi::GetAnimationVariablesFromActor(
  const JsFunctionArguments& args)
{
  const auto actorFormId =
    static_cast<RE::FormID>(static_cast<double>(args[1]));

  const auto pActor = RE::TESForm::LookupByID<RE::Actor>(actorFormId);

  if (!pActor) {
    return JsValue::Undefined();
  }

  const auto animVariables =
    AnimationGraphMasterBehaviourDescriptor{ *pActor }.GetVariables();

  auto obj = JsValue::Object();

  AddObjProperty(
    &obj, "booleans",
    reinterpret_cast<const uint8_t*>(animVariables.booleans.data()),
    animVariables.SizeBooleansInBytes());

  AddObjProperty(&obj, "floats",
                 reinterpret_cast<const uint8_t*>(animVariables.floats.data()),
                 animVariables.SizeFloatsInBytes());

  AddObjProperty(
    &obj, "integers",
    reinterpret_cast<const uint8_t*>(animVariables.integers.data()),
    animVariables.SizeIntegersInBytes());

  return obj;
}

JsValue MagicApi::ApplyAnimationVariablesToActor(
  const JsFunctionArguments& args)
{
  const auto actorFormId =
    static_cast<RE::FormID>(static_cast<double>(args[1]));

  const auto pActor = RE::TESForm::LookupByID<RE::Actor>(actorFormId);

  if (!pActor) {
    return JsValue::Bool(false);
  }

  const bool isAnimationVariablesApplied =
    AnimationGraphMasterBehaviourDescriptor{
      skymp::magic::details::GetAnimationVariablesFromJSArg(args[2])
    }
      .ApplyVariablesToActor(*pActor);

  return JsValue::Bool(isAnimationVariablesApplied);
}

void MagicApi::Register(JsValue& exports)
{
  exports.SetProperty("castSpellImmediate",
                      JsValue::Function(CastSpellImmediate));
  exports.SetProperty("interruptCast", JsValue::Function(InterruptCast));

  exports.SetProperty("getAnimationVariablesFromActor",
                      JsValue::Function(GetAnimationVariablesFromActor));

  exports.SetProperty("applyAnimationVariablesToActor",
                      JsValue::Function(ApplyAnimationVariablesToActor));
}
