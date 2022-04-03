#pragma once

class VmCallback : public RE::BSScript::IStackCallbackFunctor
{
public:
  using OnResult = std::function<void(const Variable& result)>;

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

  void operator()(Variable result) override
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
