namespace VM {

class CallbackFunctor : public RE::BSScript::IStackCallbackFunctor
{
  using ResultCallback = std::function<void(const Variable& result)>;

public:
  static auto New(const ResultCallback& onResult)
  {
    RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> res;
    res.reset(new CallbackFunctor(onResult));
    return res;
  }

private:
  CallbackFunctor(const ResultCallback& onResult)
    : callback(onResult)
  {
  }

  void operator()(Variable result) override
  {
    if (callback)
      callback(result);
  }

  bool CanSave() const override { return false; }

  void SetObject(
    const RE::BSTSmartPointer<RE::BSScript::Object>& object) override
  {
  }

  const ResultCallback callback;
};

bool CallClassFunction(
  IVM* vm, FixedString className, FixedString funcName,
  RE::BSScript::IFunctionArguments* funcArgs,
  RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callbackFunctor)
{
  try {
    return vm->DispatchStaticCall(className, funcName, funcArgs,
                                  callbackFunctor);
  } catch (const std::exception& e) {
    logger::critical("CallClassFunction: ", e.what());
    return false;
  }
}

bool CallObjectFunction(
  IVM* vm, RE::BSTSmartPointer<RE::BSScript::Object> obj, FixedString funcName,
  RE::BSScript::IFunctionArguments* funcArgs,
  RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callbackFunctor)
{
  try {
    return vm->DispatchMethodCall(obj, funcName, funcArgs, callbackFunctor);
  } catch (const std::exception& e) {
    logger::critical("CallObjectFunction: ", e.what());
    return false;
  }
}

bool CallHandleFunction(
  IVM* vm, RE::VMHandle handle, FixedString className, FixedString funcName,
  RE::BSScript::IFunctionArguments* funcArgs,
  RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callbackFunctor)
{
  try {
    return vm->DispatchMethodCall(handle, className, funcName, funcArgs,
                                  callbackFunctor);
  } catch (const std::exception& e) {
    logger::critical("CallHandleFunction: ", e.what());
    return false;
  }
}

}
