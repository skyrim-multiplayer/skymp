#include "NullPointerException.h"
#include "ObjectReferenceApi.h"

RE::TESObjectREFR* GetArgObjectReference(const JsValue& arg)
{
  auto formId = static_cast<uint32_t>(static_cast<double>(arg));
  auto refr = RE::TESForm::LookupByID<RE::TESObjectREFR>(formId);

  if (!refr) {
    throw NullPointerException("refr");
  }

  return refr;
}

JsValue ActorApi::SetCollision(const JsFunctionArguments& args)
{
  auto refr = GetArgObjectReference(args[1]);
  refr->SetCollision(static_cast<bool>(args[2]));
  return JsValue::Undefined();
}
