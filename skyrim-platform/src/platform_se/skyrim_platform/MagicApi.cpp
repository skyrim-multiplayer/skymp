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

  const auto formIdTarget = RE::TESForm::LookupByID<RE::TESObjectREFR>(
    static_cast<uint32_t>(static_cast<double>(args[4])));

  g_nativeCallRequirements.gameThrQ->AddTask(
    [spellFormId, actorFormId, castingSource, formIdTarget,
     animVars =
       skymp::magic::details::GetAnimationVariablesFromJSArg(args[5])]() {
      const auto pSpell = RE::TESForm::LookupByID<RE::MagicItem>(spellFormId);

      const auto pActor = RE::TESForm::LookupByID<RE::Actor>(actorFormId);

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

      const auto magicCaster = pActor->GetMagicCaster(castingSource);

      if (!magicCaster) {
        return;
      }

      const bool isAnimationVariablesApplied =
        AnimationGraphMasterBehaviourDescriptor{ std::move(animVars) }
          .ApplyVariablesToActor(*pActor);

      if (!isAnimationVariablesApplied) {
        return;
      }

      magicCaster->CastSpellImmediate(pSpell, false, formIdTarget, 1.0f, false,
                                      0.0f, pActor);
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
