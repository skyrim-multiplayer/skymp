#include "NullPointerException.h"

namespace FenixFunctions {

namespace Impl {

void Sit(RE::Actor* a)
{
  a->actorState1.sitSleepState = RE::SIT_SLEEP_STATE::kIsSitting;
}

void GetUp(RE::Actor* a)
{
  a->actorState1.sitSleepState = RE::SIT_SLEEP_STATE::kNormal;
}

enum class LineOfSightLocation : uint32_t
{
  kNone,
  kEyes,
  kHead,
  kTorso,
  kFeet
};

RE::NiPoint3 CalculateLOSLocation(RE::TESObjectREFR* refr,
                                  LineOfSightLocation los_loc)
{
  using func_t = decltype(CalculateLOSLocation);
  REL::Relocation<func_t> func{ RELOCATION_ID(46021, 44877) };
  return func(refr, los_loc);
}

RE::NiPoint3 ConvertAnglesToDir(const RE::NiPoint3& angles)
{
  RE::NiPoint3 ans;

  float sinx = sinf(angles.x);
  float cosx = cosf(angles.x);
  float sinz = sinf(angles.z);
  float cosz = cosf(angles.z);

  ans.x = cosx * sinz;
  ans.y = cosx * cosz;
  ans.z = -sinx;

  return ans;
}

RE::NiPoint3 Rotate(float r, const RE::NiPoint3& angles)
{
  return ConvertAnglesToDir(angles) * r;
}

std::pair<RE::NiPoint3, RE::NiPoint3> RaycastActor(RE::Actor* caster, float R)
{
  auto havokWorldScale = RE::bhkWorld::GetWorldScale();
  RE::bhkPickData pickData;
  RE::NiPoint3 rayStart, rayEnd;

  rayStart = CalculateLOSLocation(caster, LineOfSightLocation::kHead);
  rayEnd = rayStart + Rotate(R, caster->data.angle);
  pickData.rayInput.from = rayStart * havokWorldScale;
  pickData.rayInput.to = rayEnd * havokWorldScale;

  uint32_t collisionFilterInfo = 0;
  caster->GetCollisionFilterInfo(collisionFilterInfo);
  pickData.rayInput.filterInfo =
    (static_cast<uint32_t>(collisionFilterInfo >> 16) << 16) |
    static_cast<uint32_t>(RE::COL_LAYER::kCharController);

  caster->GetParentCell()->GetbhkWorld()->PickObject(pickData);
  RE::NiPoint3 hitPos;
  if (pickData.rayOutput.HasHit()) {
    hitPos = rayStart + (rayEnd - rayStart) * pickData.rayOutput.hitFraction;
    // pick_data.rayOutput.normal;
    RE::NiPoint3 normal = { pickData.rayOutput.normal.quad.m128_f32[0],
                            pickData.rayOutput.normal.quad.m128_f32[1],
                            pickData.rayOutput.normal.quad.m128_f32[2] };
    return { hitPos, normal };
  } else {
    return { rayEnd, {} };
  }
}

void CalculateAnticipatedLocation(RE::TESObjectREFR* refr, float dtime,
                                  RE::NiPoint3& ans)
{
  using func_t = decltype(CalculateAnticipatedLocation);
  REL::Relocation<func_t> func{ RELOCATION_ID(46045, 47309) };
  return func(refr, dtime, ans);
}

}

RE::Actor* GetArgActor(const JsValue& arg)
{
  auto formId = static_cast<uint32_t>(static_cast<double>(arg));
  auto a = RE::TESForm::LookupByID<RE::Actor>(formId);

  if (!a) {
    throw NullPointerException("pActor");
  }

  return a;
}

JsValue ConvertPointToJS(const RE::NiPoint3& P)
{
  std::vector<JsValue> p = { P.x, P.y, P.z };
  return p;
}

JsValue ActorSit(const JsFunctionArguments& args)
{
  auto a = GetArgActor(args[1]);
  Impl::Sit(a);
  return JsValue::Undefined();
}

JsValue ActorGetUp(const JsFunctionArguments& args)
{
  auto a = GetArgActor(args[1]);
  Impl::GetUp(a);
  return JsValue::Undefined();
}

JsValue ActorRaycast(const JsFunctionArguments& args)
{
  auto a = GetArgActor(args[1]);
  float R = static_cast<float>(static_cast<double>(args[2]));
  auto [P, N] = Impl::RaycastActor(a, R);
  std::vector<JsValue> ans = { P.x, P.y, P.z, N.x, N.y, N.z };
  return ans;
}

JsValue CalculateAnticipatedLocation(const JsFunctionArguments& args)
{
  auto a = GetArgActor(args[1]);
  float dTime = static_cast<float>(static_cast<double>(args[2]));

  RE::NiPoint3 ans;
  Impl::CalculateAnticipatedLocation(a, dTime, ans);

  return ConvertPointToJS(ans);
}

void Register(JsValue& exports)
{
  exports.SetProperty("actorSit", JsValue::Function(ActorSit));
  exports.SetProperty("actorGetUp", JsValue::Function(ActorGetUp));
  exports.SetProperty("actorRaycast", JsValue::Function(ActorRaycast));
  exports.SetProperty("calculateAnticipatedLocation",
                      JsValue::Function(CalculateAnticipatedLocation));
}

}
