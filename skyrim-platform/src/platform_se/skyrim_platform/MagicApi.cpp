#include "MagicApi.h"
#include "CallNativeApi.h"
#include "Magic/AnimationGraphMasterBehaviourDescriptor.h"
#include "NullPointerException.h"

JsValue MagicApi::CastSpellImmediate(const JsFunctionArguments& args)
{
  const auto actorFormId =
    static_cast<RE::FormID>(static_cast<double>(args[1]));

  const auto pActor = RE::TESForm::LookupByID<RE::Actor>(actorFormId);

  const auto castingSource =
    static_cast<RE::MagicSystem::CastingSource>(static_cast<int>(args[2]));

  const auto spellFormId =
    static_cast<RE::FormID>(static_cast<double>(args[3]));

  const auto pSpell = RE::TESForm::LookupByID<RE::MagicItem>(spellFormId);

  if (!pSpell) {
    return JsValue::Undefined();
  }

  const auto t = pSpell->GetFormType();

  if (!(t == RE::FormType::Spell || t == RE::FormType::Scroll ||
        t == RE::FormType::Ingredient || t == RE::FormType::AlchemyItem ||
        t == RE::FormType::Enchantment)) {
    return JsValue::Undefined();
  }

  const auto formIdTarget = RE::TESForm::LookupByID<RE::TESObjectREFR>(
    static_cast<uint32_t>(static_cast<double>(args[4])));

  if (!pActor) {
    return JsValue::Undefined();
  }

  const auto magicCaster = pActor->GetMagicCaster(castingSource);

  if (!magicCaster) {
    return JsValue::Undefined();
  }

  using AnimVarInitializer =
    AnimationGraphMasterBehaviourDescriptor::AnimationVariables::InitData;

  const auto variables =
    AnimationGraphMasterBehaviourDescriptor::AnimationVariables{
      AnimVarInitializer{ static_cast<uint8_t*>(args[5].GetTypedArrayData()),
                          args[5].GetTypedArrayBufferLength() },
      AnimVarInitializer{
        static_cast<uint8_t*>(args[6].GetTypedArrayData()),
        static_cast<size_t>(args[6].GetTypedArrayBufferLength()) },
      AnimVarInitializer{ static_cast<uint8_t*>(args[7].GetTypedArrayData()),
                          args[7].GetTypedArrayBufferLength() }
    };

  const bool isAnimationVariablesApplied =
    AnimationGraphMasterBehaviourDescriptor{ std::move(variables) }
      .ApplyVariablesToActor(*pActor);

  if (!isAnimationVariablesApplied) {
    return JsValue::Undefined();
  }

  magicCaster->CastSpellImmediate(pSpell, false, formIdTarget, 1.0f, false,
                                  0.0f, pActor);

  return JsValue::Undefined();
}

JsValue MagicApi::InterruptCast(const JsFunctionArguments& args)
{
  const auto actorFormId =
    static_cast<RE::FormID>(static_cast<double>(args[1]));

  const auto pActor = RE::TESForm::LookupByID<RE::Actor>(actorFormId);

  if (!pActor) {
    return JsValue::Undefined();
  }

  using AnimVarInitializer =
    AnimationGraphMasterBehaviourDescriptor::AnimationVariables::InitData;

  const auto variables =
    AnimationGraphMasterBehaviourDescriptor::AnimationVariables{
      AnimVarInitializer{ static_cast<uint8_t*>(args[3].GetTypedArrayData()),
                          args[3].GetTypedArrayBufferLength() },
      AnimVarInitializer{ static_cast<uint8_t*>(args[4].GetTypedArrayData()),
                          args[4].GetTypedArrayBufferLength() },
      AnimVarInitializer{ static_cast<uint8_t*>(args[5].GetTypedArrayData()),
                          args[5].GetTypedArrayBufferLength() }
    };

  const bool isAnimationVariablesApplied =
    AnimationGraphMasterBehaviourDescriptor{ std::move(variables) }
      .ApplyVariablesToActor(*pActor);

  if (!isAnimationVariablesApplied) {
    return JsValue::Undefined();
  }

  const bool restoreMagic = args[2];
  pActor->InterruptCast(restoreMagic);

  return JsValue::Undefined();
}

void MagicApi::Register(JsValue& exports)
{
  exports.SetProperty("castSpellImmediate",
                      JsValue::Function(CastSpellImmediate));
  exports.SetProperty("interruptCast", JsValue::Function(InterruptCast));
}
