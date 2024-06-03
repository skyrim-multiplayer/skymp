#include "NullPointerException.h"

namespace FenixFunctions {

namespace Impl {

void sit(RE::Actor* a)
{
  a->actorState1.sitSleepState = RE::SIT_SLEEP_STATE::kIsSitting;
}

void getUp(RE::Actor* a)
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

RE::NiPoint3 angles2dir(const RE::NiPoint3& angles)
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

RE::NiPoint3 rotate(float r, const RE::NiPoint3& angles)
{
  return angles2dir(angles) * r;
}

RE::NiPoint3 raycast_actor(RE::Actor* caster, float R)
{
  auto havokWorldScale = RE::bhkWorld::GetWorldScale();
  RE::bhkPickData pick_data;
  RE::NiPoint3 ray_start, ray_end;

  ray_start = CalculateLOSLocation(caster, LineOfSightLocation::kHead);
  ray_end = ray_start + rotate(R, caster->data.angle);
  pick_data.rayInput.from = ray_start * havokWorldScale;
  pick_data.rayInput.to = ray_end * havokWorldScale;

  uint32_t collisionFilterInfo = 0;
  caster->GetCollisionFilterInfo(collisionFilterInfo);
  pick_data.rayInput.filterInfo =
    (static_cast<uint32_t>(collisionFilterInfo >> 16) << 16) |
    static_cast<uint32_t>(RE::COL_LAYER::kCharController);

  caster->GetParentCell()->GetbhkWorld()->PickObject(pick_data);
  RE::NiPoint3 hitpos;
  if (pick_data.rayOutput.HasHit()) {
    hitpos =
      ray_start + (ray_end - ray_start) * pick_data.rayOutput.hitFraction;
    // pick_data.rayOutput.normal;
  } else {
    hitpos = ray_end;
  }
  return hitpos;
}

void CalculateAnticipatedLocation(RE::TESObjectREFR* refr, float dtime,
                                  RE::NiPoint3& ans)
{
  using func_t = decltype(CalculateAnticipatedLocation);
  REL::Relocation<func_t> func{ RELOCATION_ID(46045, 47309) };
  return func(refr, dtime, ans);
}

}

RE::Actor* get_arg_actor(const JsValue& arg)
{
  auto formId = static_cast<uint32_t>(static_cast<double>(arg));
  auto a = RE::TESForm::LookupByID<RE::Actor>(formId);

  if (!a) {
    throw NullPointerException("pActor");
  }

  return a;
}

JsValue point2js(const RE::NiPoint3& P)
{
  std::vector<JsValue> p = { P.x, P.y, P.z };
  return p;
}

JsValue ActorSit(const JsFunctionArguments& args)
{
  auto a = get_arg_actor(args[1]);
  Impl::sit(a);
  return JsValue::Undefined();
}

JsValue ActorGetUp(const JsFunctionArguments& args)
{
  auto a = get_arg_actor(args[1]);
  Impl::getUp(a);
  return JsValue::Undefined();
}

JsValue ActorRaycast(const JsFunctionArguments& args)
{
  auto a = get_arg_actor(args[1]);
  float R = static_cast<float>(static_cast<double>(args[2]));
  return point2js(Impl::raycast_actor(a, R));
}

JsValue CalculateAnticipatedLocation(const JsFunctionArguments& args)
{
  auto a = get_arg_actor(args[1]);
  float dtime = static_cast<float>(static_cast<double>(args[2]));

  RE::NiPoint3 ans;
  Impl::CalculateAnticipatedLocation(a, dtime, ans);

  return point2js(ans);
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
