#pragma once

#include "AnimVariableMasterGraphIndexes.h"
#include "hkbVariableValueSet.h"

#include <type_traits>
#include <vector>

class AnimationGraphMasterBehaviourDescriptor final
{

public:
  struct AnimationVariables final
  {
  public:
    std::vector<uint8_t> booleans{};
    std::vector<float> floats{};
    std::vector<uint32_t> integers{};

  public:
    struct InitData final
    {
      uint8_t* ptr = nullptr;
      size_t numBytes = 0;
    };

    using BooleanAnimVarType = decltype(booleans)::value_type;
    using FloatAnimVarType = decltype(floats)::value_type;
    using IntegerAnimVarType = decltype(integers)::value_type;

    AnimationVariables() = default;

    explicit AnimationVariables(InitData booleanInitData,
                                InitData floatInitData,
                                InitData integerInitData)
    {
      if (booleanInitData.numBytes > 0 && booleanInitData.ptr) {
        const size_t numBooleans =
          booleanInitData.numBytes / sizeof(BooleanAnimVarType);

        booleans.reserve(numBooleans);

        for (size_t i = 0; i < numBooleans; ++i) {
          booleans.emplace_back(*reinterpret_cast<BooleanAnimVarType*>(
            booleanInitData.ptr + (i * sizeof(BooleanAnimVarType))));
        }
      }

      if (floatInitData.numBytes > 0 && floatInitData.ptr) {
        const size_t numFloats =
          floatInitData.numBytes / sizeof(FloatAnimVarType);

        floats.reserve(numFloats);

        for (size_t i = 0; i < numFloats; ++i) {
          floats.emplace_back(*reinterpret_cast<FloatAnimVarType*>(
            floatInitData.ptr + (i * sizeof(FloatAnimVarType))));
        }
      }

      if (integerInitData.numBytes > 0 && integerInitData.ptr) {
        const size_t numIntegers =
          integerInitData.numBytes / sizeof(IntegerAnimVarType);

        integers.reserve(numIntegers);

        for (size_t i = 0; i < numIntegers; ++i) {
          integers.emplace_back(*reinterpret_cast<IntegerAnimVarType*>(
            integerInitData.ptr + (i * sizeof(IntegerAnimVarType))));
        }
      }
    }

    [[nodiscard]] bool IsEmpty() const noexcept
    {
      return booleans.empty() && floats.empty() && integers.empty();
    }

    [[nodiscard]] uint32_t SizeBooleansInBytes() const noexcept
    {
      return booleans.size() * sizeof(BooleanAnimVarType);
    }

    [[nodiscard]] uint32_t SizeFloatsInBytes() const noexcept
    {
      return floats.size() * sizeof(FloatAnimVarType);
    }

    [[nodiscard]] uint32_t SizeIntegersInBytes() const noexcept
    {
      return integers.size() * sizeof(IntegerAnimVarType);
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

    variables.booleans.resize(agDescriptor.boolVariableIndexes.size());
    variables.floats.resize(agDescriptor.floatVariableIndexes.size());
    variables.integers.resize(agDescriptor.intVariableIndexes.size());

    for (size_t i = 0; i < agDescriptor.boolVariableIndexes.size(); ++i) {
      variables.booleans[i] =
        *reinterpret_cast<AnimationVariables::BooleanAnimVarType*>(
          &pVariableSet->varSet[agDescriptor.boolVariableIndexes[i]]);
    }

    for (size_t i = 0; i < agDescriptor.floatVariableIndexes.size(); ++i) {
      variables.floats[i] =
        *reinterpret_cast<AnimationVariables::FloatAnimVarType*>(
          &pVariableSet->varSet[agDescriptor.floatVariableIndexes[i]]);
    }

    for (size_t i = 0; i < agDescriptor.intVariableIndexes.size(); ++i) {
      variables.integers[i] =
        *reinterpret_cast<AnimationVariables::IntegerAnimVarType*>(
          &pVariableSet->varSet[agDescriptor.intVariableIndexes[i]]);
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

    for (size_t i = 0; i < agDescriptor.boolVariableIndexes.size(); ++i) {
      *reinterpret_cast<AnimationVariables::BooleanAnimVarType*>(
        &pVariableSet->varSet[agDescriptor.boolVariableIndexes[i]]) =
        variables.booleans.size() > i ? variables.booleans[i] : 0;
    }

    for (size_t i = 0; i < agDescriptor.floatVariableIndexes.size(); ++i) {
      *reinterpret_cast<AnimationVariables::FloatAnimVarType*>(
        &pVariableSet->varSet[agDescriptor.floatVariableIndexes[i]]) =
        variables.floats.size() > i ? variables.floats[i] : 0.f;
    }

    for (size_t i = 0; i < agDescriptor.intVariableIndexes.size(); ++i) {
      *reinterpret_cast<AnimationVariables::IntegerAnimVarType*>(
        &pVariableSet->varSet[agDescriptor.intVariableIndexes[i]]) =
        variables.integers.size() > i ? variables.integers[i] : 0;
    }

    return true;
  }

private:
  AnimationVariables variables{};

  AnimVariableMasterGraphIndexes agDescriptor =
    AnimVariableMasterGraphIndexes::CreateDefault();
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
                              AnimationVariables::booleans)>::value,
  "Member of AnimationVariables->Booleans must be std::vector<T>");

static_assert(
  skymp::is_vector<decltype(AnimationGraphMasterBehaviourDescriptor::
                              AnimationVariables::floats)>::value,
  "Member of AnimationVariables->Floats must be std::vector<T>");

static_assert(
  skymp::is_vector<decltype(AnimationGraphMasterBehaviourDescriptor::
                              AnimationVariables::integers)>::value,
  "Member of AnimationVariables->Integers must be std::vector<T>");
