#include "SpawnPointBinding.h"

Napi::Value SpawnPointBinding::Get(Napi::Env env, ScampServer& scampServer,
                                   uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  LocationalData spawnPoint = actor.GetSpawnPoint();

  auto locationalData = Napi::Object::New(env);

  locationalData.Set(
    "cellOrWorldDesc",
    Napi::String::New(env, spawnPoint.cellOrWorldDesc.ToString()));

  auto& niPoint3 = spawnPoint.pos;
  auto arr = Napi::Array::New(env, 3);
  arr.Set(uint32_t(0), Napi::Number::New(env, niPoint3.x));
  arr.Set(uint32_t(1), Napi::Number::New(env, niPoint3.y));
  arr.Set(uint32_t(2), Napi::Number::New(env, niPoint3.z));
  locationalData.Set("pos", arr);

  auto& niPoint3Angle = spawnPoint.rot;
  auto arrAngle = Napi::Array::New(env, 3);
  arrAngle.Set(uint32_t(0), Napi::Number::New(env, niPoint3Angle.x));
  arrAngle.Set(uint32_t(1), Napi::Number::New(env, niPoint3Angle.y));
  arrAngle.Set(uint32_t(2), Napi::Number::New(env, niPoint3Angle.z));
  locationalData.Set("rot", arrAngle);

  return locationalData;
}

void SpawnPointBinding::Apply(MpActor& actor,
                              const LocationalData& locationalData)
{
  actor.SetSpawnPoint(locationalData);
}
