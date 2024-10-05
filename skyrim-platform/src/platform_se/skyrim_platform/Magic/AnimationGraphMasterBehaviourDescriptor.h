#pragma once

#include "AnimationGraphDescriptor.h"
#include "AnimationVariablesForMasterBehavior.h"
#include "hkbVariableValueSet.h"

#include <type_traits>
#include <vector>

class AnimationGraphMasterBehaviourDescriptor final
{

public:
  struct AnimationVariables final
  {
  public:
    std::vector<uint8_t> Booleans{};
    std::vector<float> Floats{};
    std::vector<uint32_t> Integers{};

  public:
    struct InitData final
    {
      uint8_t* ptr = nullptr;
      size_t numBytes = 0;
    };

    using BooleanAnimVarType = decltype(Booleans)::value_type;
    using FloatAnimVarType = decltype(Floats)::value_type;
    using IntegerAnimVarType = decltype(Integers)::value_type;

    AnimationVariables() = default;

    explicit AnimationVariables(InitData booleanInitData,
                                InitData floatInitData,
                                InitData integerInitData)
    {
      if (booleanInitData.numBytes > 0 && booleanInitData.ptr) {
        const size_t numBooleans =
          booleanInitData.numBytes / sizeof(BooleanAnimVarType);

        Booleans.reserve(numBooleans);

        for (size_t i = 0; i < numBooleans; ++i) {
          Booleans.emplace_back(*reinterpret_cast<BooleanAnimVarType*>(
            booleanInitData.ptr + (i * sizeof(BooleanAnimVarType))));
        }
      }

      if (floatInitData.numBytes > 0 && floatInitData.ptr) {
        const size_t numFloats =
          floatInitData.numBytes / sizeof(FloatAnimVarType);

        Floats.reserve(numFloats);

        for (size_t i = 0; i < numFloats; ++i) {
          Floats.emplace_back(*reinterpret_cast<FloatAnimVarType*>(
            floatInitData.ptr + (i * sizeof(FloatAnimVarType))));
        }
      }

      if (integerInitData.numBytes > 0 && integerInitData.ptr) {
        const size_t numIntegers =
          integerInitData.numBytes / sizeof(IntegerAnimVarType);

        Integers.reserve(numIntegers);

        for (size_t i = 0; i < numIntegers; ++i) {
          Integers.emplace_back(*reinterpret_cast<IntegerAnimVarType*>(
            integerInitData.ptr + (i * sizeof(IntegerAnimVarType))));
        }
      }
    }

    [[nodiscard]] bool IsEmpty() const noexcept
    {
      return Booleans.empty() && Floats.empty() && Integers.empty();
    }

    [[nodiscard]] uint32_t SizeBooleansInBytes() const noexcept
    {
      return Booleans.size() * sizeof(BooleanAnimVarType);
    }

    [[nodiscard]] uint32_t SizeFloatsInBytes() const noexcept
    {
      return Floats.size() * sizeof(FloatAnimVarType);
    }

    [[nodiscard]] uint32_t SizeIntegersInBytes() const noexcept
    {
      return Integers.size() * sizeof(IntegerAnimVarType);
    }
  };

  explicit AnimationGraphMasterBehaviourDescriptor(
    AnimationVariables _variables)
    : variables(std::move(_variables))
  {
  }

  AnimationVariables GetVariables() const { return variables; }

  explicit AnimationGraphMasterBehaviourDescriptor(const RE::Actor& actor)
  {
    RE::BSTSmartPointer<RE::BSAnimationGraphManager> pManager;

    if (actor.GetAnimationGraphManager(pManager) == false) {
      return;
    }

    RE::BSSpinLockGuard _{ pManager->updateLock };

    if (pManager->activeGraph >= pManager->graphs.size()) {
      return;
    }

    const RE::BShkbAnimationGraph* pGraph = actor.formID == 0x14
      ? pManager->graphs[0].get()
      : pManager->graphs[pManager->activeGraph].get();

    if (pGraph == nullptr || pGraph->behaviorGraph == nullptr ||
        pGraph->behaviorGraph->rootGenerator.get() == nullptr ||
        pGraph->behaviorGraph->rootGenerator->name.data() == nullptr) {
      return;
    }

    const auto* pVariableSet =
      reinterpret_cast<hkbVariableValueSet<uint32_t>*>(
        pGraph->behaviorGraph->variableValueSet.get());

    if (!pVariableSet) {
      return;
    }

    variables.Booleans.resize(agDescriptor.BooleanLookUpTable.size());
    variables.Floats.resize(agDescriptor.FloatLookupTable.size());
    variables.Integers.resize(agDescriptor.IntegerLookupTable.size());

    for (size_t i = 0; i < agDescriptor.BooleanLookUpTable.size(); ++i) {
      variables.Booleans[i] =
        *reinterpret_cast<AnimationVariables::BooleanAnimVarType*>(
          &pVariableSet->data[agDescriptor.BooleanLookUpTable[i]]);
    }

    for (size_t i = 0; i < agDescriptor.FloatLookupTable.size(); ++i) {
      variables.Floats[i] =
        *reinterpret_cast<AnimationVariables::FloatAnimVarType*>(
          &pVariableSet->data[agDescriptor.FloatLookupTable[i]]);
    }

    for (size_t i = 0; i < agDescriptor.IntegerLookupTable.size(); ++i) {
      variables.Integers[i] =
        *reinterpret_cast<AnimationVariables::IntegerAnimVarType*>(
          &pVariableSet->data[agDescriptor.IntegerLookupTable[i]]);
    }
  }

  [[nodiscard]] bool ApplyVariablesToActor(const RE::Actor& actor) const
  {
    if (variables.IsEmpty()) {
      return false;
    }

    RE::BSTSmartPointer<RE::BSAnimationGraphManager> pManager;

    if (actor.GetAnimationGraphManager(pManager) == false) {
      return false;
    }

    RE::BSSpinLockGuard _{ pManager->updateLock };

    if (pManager->activeGraph >= pManager->graphs.size()) {
      return false;
    }

    const RE::BShkbAnimationGraph* pGraph = actor.formID == 0x14
      ? pManager->graphs[0].get()
      : pManager->graphs[pManager->activeGraph].get();

    if (pGraph == nullptr || pGraph->behaviorGraph == nullptr ||
        pGraph->behaviorGraph->rootGenerator.get() == nullptr ||
        pGraph->behaviorGraph->rootGenerator->name.data() == nullptr) {
      return false;
    }

    const auto* pVariableSet =
      reinterpret_cast<hkbVariableValueSet<uint32_t>*>(
        pGraph->behaviorGraph->variableValueSet.get());

    if (!pVariableSet) {
      return false;
    }

    for (size_t i = 0; i < agDescriptor.BooleanLookUpTable.size(); ++i) {
      *reinterpret_cast<AnimationVariables::BooleanAnimVarType*>(
        &pVariableSet->data[agDescriptor.BooleanLookUpTable[i]]) =
        variables.Booleans.size() > i ? variables.Booleans[i] : 0;
    }

    for (size_t i = 0; i < agDescriptor.FloatLookupTable.size(); ++i) {
      *reinterpret_cast<AnimationVariables::FloatAnimVarType*>(
        &pVariableSet->data[agDescriptor.FloatLookupTable[i]]) =
        variables.Floats.size() > i ? variables.Floats[i] : 0.f;
    }

    for (size_t i = 0; i < agDescriptor.IntegerLookupTable.size(); ++i) {
      *reinterpret_cast<AnimationVariables::IntegerAnimVarType*>(
        &pVariableSet->data[agDescriptor.IntegerLookupTable[i]]) =
        variables.Integers.size() > i ? variables.Integers[i] : 0;
    }

    return true;
  }

private:
  AnimationVariables variables;

  const AnimationGraphDescriptor agDescriptor = AnimationGraphDescriptor{
    { kbEquipOk,
      kbMotionDriven,
      kIsBeastRace,
      kIsSneaking,
      kIsBleedingOut,
      kIsCastingDual,
      kIs1HM,
      kIsCastingRight,
      kIsCastingLeft,
      kIsBlockHit,
      kIsPlayer,
      kIsNPC,
      kbIsSynced,
      kbVoiceReady,
      kbWantCastLeft,
      kbWantCastRight,
      kbWantCastVoice,
      kb1HM_MLh_attack,
      kb1HMCombat,
      kbAnimationDriven,
      kbCastReady,
      kIsAttacking,
      kbAllowRotation,
      kbMagicDraw,
      kbMLh_Ready,
      kbMRh_Ready,
      kbInMoveState,
      kbSprintOK,
      kbIdlePlaying,
      kbIsDialogueExpressive,
      kbAnimObjectLoaded,
      kbEquipUnequip,
      kbAttached,
      kbIsH2HSolo,
      kbHeadTracking,
      kbIsRiding,
      kbTalkable,
      kbRitualSpellActive,
      kbInJumpState,
      kbHeadTrackSpine,
      kbLeftHandAttack,
      kbIsInMT,
      kbHumanoidFootIKEnable,
      kbHumanoidFootIKDisable,
      kbStaggerPlayerOverride,
      kbNoStagger,
      kbIsStaffLeftCasting,
      kbPerkShieldCharge,
      kbPerkQuickShot,
      kIsBlocking,
      kIsBashing,
      kIsStaggering,
      kIsRecoiling,
      kIsEquipping,
      kIsUnequipping,
      kisInFurniture,
      kbNeutralState,
      kbBowDrawn,
      kPitchOverride,
      kNotCasting },

    { kTurnDelta, kDirection, kSpeedSampled, kweapAdj, kSpeed, kCastBlend,
      kPitchOffset, kSpeedDamped, kPitch, kVelocityZ, k1stPRot, k1stPRotDamped,
      kCastBlendDamped },

    { kiRightHandEquipped, kiLeftHandEquipped, ki1HMState, kiState,
      kiLeftHandType, kiRightHandType, kiSyncIdleLocomotion,
      kiSyncForwardState, kiSyncTurnState, kiIsInSneak, kiWantBlock,
      kiRegularAttack, ktestint, kcurrentDefaultState }
  };
};

namespace skymp {
template <typename T>
struct is_vector : std::false_type
{
};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type
{
};
} // namespace skymp

static_assert(
  skymp::is_vector<decltype(AnimationGraphMasterBehaviourDescriptor::
                              AnimationVariables::Booleans)>::value,
  "Member of AnimationVariables->Booleans must be std::vector<T>");

static_assert(
  skymp::is_vector<decltype(AnimationGraphMasterBehaviourDescriptor::
                              AnimationVariables::Floats)>::value,
  "Member of AnimationVariables->Floats must be std::vector<T>");

static_assert(
  skymp::is_vector<decltype(AnimationGraphMasterBehaviourDescriptor::
                              AnimationVariables::Integers)>::value,
  "Member of AnimationVariables->Integers must be std::vector<T>");
