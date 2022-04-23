#include "GetNativeFunctionAddr.h"

GetNativeFunctionAddr::Result GetNativeFunctionAddr::Run(
  const RE::BSScript::IFunction& f)
{
  if (!f.GetIsNative()) {
    return { nullptr, false, false, false };
  }

  auto addrPtr = reinterpret_cast<const size_t*>(
    reinterpret_cast<const uint8_t*>(&f) +
    sizeof(RE::BSScript::NF_util::NativeFunctionBase));
  auto nativeFn = reinterpret_cast<void*>(*addrPtr);

  auto useLongSignaturePtr = reinterpret_cast<const bool*>(
    reinterpret_cast<const uint8_t*>(addrPtr) + sizeof(void*));

  auto isLatentPtr =
    reinterpret_cast<const bool*>(&(const RE::BSScript::IFunction&)f) + 0x42;

  return { nativeFn, *useLongSignaturePtr, *isLatentPtr, true };
}
