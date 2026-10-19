#include "ObjectReferenceApi.h"

#include "NullPointerException.h"

#include <RE/B/BSContainer.h>
#include <RE/E/ExtraMapMarker.h>
#include <RE/T/TES.h>

namespace {
RE::TESObjectREFR* GetArgObjectReference(const Napi::Value& arg)
{
  auto formId = NapiHelper::ExtractUInt32(arg, "refrFormId");
  auto refr = RE::TESForm::LookupByID<RE::TESObjectREFR>(formId);

  if (!refr) {
    throw NullPointerException("refr");
  }

  return refr;
}
}

Napi::Value ObjectReferenceApi::SetCollision(const Napi::CallbackInfo& info)
{
  auto refr = GetArgObjectReference(info[0]);
  refr->SetCollision(NapiHelper::ExtractBoolean(info[1], "collision"));
  return info.Env().Undefined();
}

Napi::Value ObjectReferenceApi::ShowMapMarker(
  const Napi::CallbackInfo& info)
{
  auto refrFormId = NapiHelper::ExtractUInt32(info[0], "refrFormId");
  bool show = NapiHelper::ExtractBoolean(info[1], "show");

  auto* refr = RE::TESForm::LookupByID<RE::TESObjectREFR>(refrFormId);
  if (!refr) {
    throw NullPointerException("refr");
  }

  auto* extra = refr->extraList.GetByType<RE::ExtraMapMarker>();
  if (!extra || !extra->mapData) {
    throw NullPointerException("mapData");
  }

  extra->mapData->SetVisible(show);
  if (show) {
    extra->mapData->flags.set(RE::MapMarkerData::Flag::kCanTravelTo);
  } else {
    extra->mapData->flags.reset(RE::MapMarkerData::Flag::kCanTravelTo);
  }

  return info.Env().Undefined();
}
