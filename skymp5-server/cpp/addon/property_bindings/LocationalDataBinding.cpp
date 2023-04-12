#include "LocationalDataBinding.h"
#include "NapiHelper.h"

Napi::Value LocationalDataBinding::Get(Napi::Env env, ScampServer& scampServer,
                                       uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  auto locationalData = Napi::Object::New(env);

  locationalData.Set("cellOrWorldDesc",
                     Napi::String::New(env, refr.GetCellOrWorld().ToString()));

  auto& niPoint3 = refr.GetPos();
  auto arr = Napi::Array::New(env, 3);
  arr.Set(uint32_t(0), Napi::Number::New(env, niPoint3.x));
  arr.Set(uint32_t(1), Napi::Number::New(env, niPoint3.y));
  arr.Set(uint32_t(2), Napi::Number::New(env, niPoint3.z));
  locationalData.Set("pos", arr);

  auto& niPoint3Angle = refr.GetAngle();
  auto arrAngle = Napi::Array::New(env, 3);
  arrAngle.Set(uint32_t(0), Napi::Number::New(env, niPoint3Angle.x));
  arrAngle.Set(uint32_t(1), Napi::Number::New(env, niPoint3Angle.y));
  arrAngle.Set(uint32_t(2), Napi::Number::New(env, niPoint3Angle.z));
  locationalData.Set("rot", arrAngle);

  return locationalData;
}

void LocationalDataBinding::Set(Napi::Env, ScampServer& scampServer,
                                uint32_t formId, Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  LocationalData locationalData;

  auto newLocationalData =
    NapiHelper::ExtractObject(newValue, "newLocationalData");
  locationalData.cellOrWorldDesc = FormDesc::FromString(
    NapiHelper::ExtractString(newLocationalData.Get("cellOrWorldDesc"),
                              "newLocationalData.cellOrWorldDesc"));
  locationalData.pos = NapiHelper::ExtractNiPoint3(
    newLocationalData.Get("pos"), "newLocationalData.pos");
  locationalData.rot = NapiHelper::ExtractNiPoint3(
    newLocationalData.Get("rot"), "newLocationalData.rot");

  if (auto actor = dynamic_cast<MpActor*>(&refr)) {
    Apply(*actor, locationalData);
  } else {
    throw std::runtime_error("mp.set can only change '" + GetPropertyName() +
                             "' for actors, not for refrs");
  }
}

void LocationalDataBinding::Apply(MpActor& actor,
                                  const LocationalData& locationalData)
{
  actor.Teleport(locationalData);
}
