#pragma once
#include <RE/BSScript/IStackCallbackFunctor.h>
#include <RE/BSScript/Variable.h>

class VmCallback : public RE::BSScript::IStackCallbackFunctor
{
public:
  using OnResult = std::function<void(const RE::BSScript::Variable& result)>;

  static auto New(const OnResult& onResult_)
  {
    RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> res;
    res.reset(new VmCallback(onResult_));
    return res;
  }

private:
  VmCallback(const OnResult& onResult_)
    : onResult(onResult_)
  {
  }

  void operator()(RE::BSScript::Variable result) override
  {
    if (onResult)
      onResult(result);
  }

  bool CanSave() const override { return false; }

  void SetObject(
    const RE::BSTSmartPointer<RE::BSScript::Object>& object) override
  {
  }

  const OnResult onResult;
};
