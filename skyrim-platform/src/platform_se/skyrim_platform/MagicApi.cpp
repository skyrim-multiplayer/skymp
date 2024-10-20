#include "MagicApi.h"
#include "CallNativeApi.h"
#include "JsUtils.h"
#include "SkyrimPlatform.h"

#include "Magic/AnimationGraphMasterBehaviourDescriptor.h"

extern CallNativeApi::NativeCallRequirements g_nativeCallRequirements;

namespace skymp::magic::details {

AnimationGraphMasterBehaviourDescriptor::AnimationVariables
GetAnimationVariablesFromJSArg(const Napi::Object& argObj)
{
  using AnimVarInitializer =
    AnimationGraphMasterBehaviourDescriptor::AnimationVariables::InitData;

  const auto booleanVarsValue = NapiHelper::ExtractUInt8Array(
    argObj.Get("booleans"), "animationVariables.booleans");

  const auto booleanVars =
    AnimVarInitializer{ static_cast<uint8_t*>(booleanVarsValue.Data()),
                        booleanVarsValue.ByteLenght() };

  const auto floatsVarsValue = NapiHelper::ExtractUInt8Array(
    argObj.Get("floats"), "animationVariables.floats");

  const auto floatsVars =
    AnimVarInitializer{ static_cast<uint8_t*>(floatsVarsValue.Data()),
                        floatsVarsValue.ByteLenght() };

  const auto integersVarsValue = NapiHelper::ExtractUInt8Array(
    argObj.Get("integers"), "animationVariables.integers");

  const auto integersVars =
    AnimVarInitializer{ static_cast<uint8_t*>(integersVarsValue.Data()),
                        integersVarsValue.ByteLenght() };

  const auto variables =
    AnimationGraphMasterBehaviourDescriptor::AnimationVariables{
      booleanVars, floatsVars, integersVars
    };

  return variables;
}

} // namespace skymp::magic::details

Napi::Value MagicApi::CastSpellImmediate(const Napi::CallbackInfo& info)
{
  const uint32_t actorFormId =
    NapiHelper::ExtractUInt32(info[0], "actorFormId");

  const RE::MagicSystem::CastingSource castingSource =
    static_cast<RE::MagicSystem::CastingSource>(
      NapiHelper::ExtractInt32(info[1], "castingSource"));

  const uint32_t spellFormId =
    NapiHelper::ExtractUInt32(info[2], "spellFormId");

  const auto formIdTarget = RE::TESForm::LookupByID<RE::TESObjectREFR>(
    NapiHelper::ExtractUInt32(info[3], "formIdTarget"));

  g_nativeCallRequirements.gameThrQ->AddTask(
    [spellFormId, actorFormId, castingSource, formIdTarget,
     animVars = skymp::magic::details::GetAnimationVariablesFromJSArg(
       NapiHelper::ExtractObject(info[4], "animationVariables"))]() {
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

  return info.Env().Undefined();
}

Napi::Value MagicApi::InterruptCast(const Napi::CallbackInfo& info)
{
  const auto actorFormId = NapiHelper::ExtractUInt32(info[0], "actorFormId");

  const auto castingSource = static_cast<RE::MagicSystem::CastingSource>(
    NapiHelper::ExtractInt32(info[1], "castingSource"));

  g_nativeCallRequirements.gameThrQ->AddTask(
    [actorFormId, castingSource,
     animVars = skymp::magic::details::GetAnimationVariablesFromJSArg(
       NapiHelper::ExtractObject(info[2], "animationVariables"))]() {
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

  return info.Env().Undefined();
}

Napi::Value MagicApi::GetAnimationVariablesFromActor(
  const Napi::CallbackInfo& info)
{
  const auto actorFormId = NapiHelper::ExtractUInt32(info[0], "actorFormId");

  const auto pActor = RE::TESForm::LookupByID<RE::Actor>(actorFormId);

  if (!pActor) {
    return info.Env().Undefined();
  }

  const auto animVariables =
    AnimationGraphMasterBehaviourDescriptor{ *pActor }.GetVariables();

  auto obj = Napi::Object::New(info.Env());

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

Napi::Value MagicApi::ApplyAnimationVariablesToActor(
  const Napi::CallbackInfo& info)
{
  const auto actorFormId = NapiHelper::ExtractUInt32(info[0], "actorFormId");

  const auto pActor = RE::TESForm::LookupByID<RE::Actor>(actorFormId);

  if (!pActor) {
    return Napi::Boolean::New(info.Env(), false);
  }

  const bool isAnimationVariablesApplied =
    AnimationGraphMasterBehaviourDescriptor{
      skymp::magic::details::GetAnimationVariablesFromJSArg(
        NapiHelper::ExtractObject(info[1], "animationVariables"))
    }
      .ApplyVariablesToActor(*pActor);

  return Napi::Boolean::New(info.Env(), isAnimationVariablesApplied);
}

void MagicApi::Register(Napi::Env env, Napi::Object& exports)
{
  exports.Set("castSpellImmediate",
              Napi::Function::New(
                env, NapiHelper::WrapCppExceptions(CastSpellImmediate)));
  exports.Set(
    "interruptCast",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(InterruptCast)));

  exports.Set(
    "getAnimationVariablesFromActor",
    Napi::Function::New(
      env, NapiHelper::WrapCppExceptions(GetAnimationVariablesFromActor)));

  exports.Set(
    "applyAnimationVariablesToActor",
    Napi::Function::New(
      env, NapiHelper::WrapCppExceptions(ApplyAnimationVariablesToActor)));
}
