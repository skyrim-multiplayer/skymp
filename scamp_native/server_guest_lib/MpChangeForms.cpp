#include "MpChangeForms.h"
#include "MpActor.h"
#include "WorldState.h"
#include <ctime>
#include <simdjson.h>

namespace {
template <class T>
T DumpToStruct(const std::string& dump)
{
  if (dump.size() > 0) {
    simdjson::dom::parser p;
    auto element = p.parse(dump).value();
    return T::FromJson(element);
  }
  return T();
}
}

std::string MpChangeForm::GetInventory() const
{
  return inv.ToJson().dump();
}

void MpChangeForm::SetInventory(const std::string& inventoryDump)
{
  inv = DumpToStruct<Inventory>(inventoryDump);
}

std::string MpChangeForm::GetEquipment() const
{
  if (equipment)
    return equipment->ToJson().dump();
  else
    return "";
}

void MpChangeForm::SetEquipment(const std::string& equipmentDump)
{
  if (equipmentDump.size() > 0)
    equipment = DumpToStruct<Equipment>(equipmentDump);
  else
    equipment.reset();
}

std::string MpChangeForm::GetFormDesc() const
{
  return formDesc.ToString();
}

void MpChangeForm::SetFormDesc(const std::string& newFormDesc)
{
  formDesc = FormDesc::FromString(newFormDesc);
}

std::string MpChangeForm::GetBaseFormDesc() const
{
  return baseDesc.ToString();
}

void MpChangeForm::SetBaseFormDesc(const std::string& newFormDesc)
{
  baseDesc = FormDesc::FromString(newFormDesc);
}

std::string MpChangeForm::GetLook() const
{
  if (look)
    return look->ToJson();
  else
    return "";
}

void MpChangeForm::SetLook(const std::string& lookDump)
{
  if (lookDump.size() > 0)
    look = DumpToStruct<Look>(lookDump);
  else
    look.reset();
}

float MpChangeForm::GetX() const
{
  return position.x;
}

void MpChangeForm::SetX(float v)
{
  position.x = v;
}

float MpChangeForm::GetY() const
{
  return position.y;
}

void MpChangeForm::SetY(float v)
{
  position.y = v;
}

float MpChangeForm::GetZ() const
{
  return position.z;
}

void MpChangeForm::SetZ(float v)
{
  position.z = v;
}

float MpChangeForm::GetAngleX() const
{
  return angle.x;
}

void MpChangeForm::SetAngleX(float v)
{
  angle.x = v;
}

float MpChangeForm::GetAngleY() const
{
  return angle.y;
}

void MpChangeForm::SetAngleY(float v)
{
  angle.y = v;
}

float MpChangeForm::GetAngleZ() const
{
  return angle.z;
}

void MpChangeForm::SetAngleZ(float v)
{
  angle.z = v;
}

/*void MpChangeForm::Load(MpChangeForm source, WorldState* parentWorldState)
{
  auto& espm = parentWorldState->GetEspm();

  uint32_t realFormId;

  if (source.shortFormId >= 0xff000000) {
    realFormId = source.shortFormId;
  } else {

    int fileIdx = -1;
    auto& names = espm.GetFileNames();
    for (int i = 0; i < names.size(); ++i) {
      if (names[i].string() == source.file) {
        fileIdx = i;
        break;
      }
    }
    if (fileIdx == -1)
      throw std::runtime_error(source.file + " not found in loaded files");

    realFormId = fileIdx * 0x01000000 + source.shortFormId;
  }

  auto& refr = parentWorldState->GetFormAt<MpObjectReference>(realFormId);
  refr.SetCellOrWorld(source.worldOrCell);
  refr.SetPos({ source.pos[0], source.pos[1], source.pos[2] });
  refr.SetAngle({ source.rot[0], source.rot[1], source.rot[2] });

  if (source.inventoryDump.size()) {
    simdjson::dom::parser p;
    auto element = p.parse(source.inventoryDump);
    refr.AddItems(Inventory::FromJson(element.value()).entries);
  }

  refr.SetHarvested(source.isHarvested);
  refr.SetOpen(source.isOpen);

  if (source.nextRelootDatetime) {
    const auto tp =
      std::chrono::system_clock::from_time_t(source.nextRelootDatetime);

    if (tp < std::chrono::system_clock::now()) {
      const auto prevRelootTime = refr.GetRelootTime();
      refr.SetRelootTime(std::chrono::duration_cast<std::chrono::milliseconds>(
        tp - std::chrono::system_clock::now()));
      refr.RequestReloot();
      refr.SetRelootTime(prevRelootTime);
    }
  }

  auto actor = dynamic_cast<MpActor*>(&refr);
  if (actor) {
    actor->SetRaceMenuOpen(source.isRaceMenuOpen);

    if (source.lookDump) {
      simdjson::dom::parser p;
      auto element = p.parse(*source.lookDump);
      auto look = Look::FromJson(element);
      actor->SetLook(&look);
    } else
      actor->SetLook(nullptr);

    if (source.equipmentDump) {
      actor->SetEquipment(*source.equipmentDump);
    }
  }
}

MpChangeForm MpChangeForm::Save(MpObjectReference* refr,
                                WorldState* parentWorldState)
{
  MpChangeForm res;
  auto& espm = parentWorldState->GetEspm();

  auto lookupRes = espm.GetBrowser().LookupById(refr->GetFormId());

  if (lookupRes.rec) {
    res.file = espm.GetFileNames()[lookupRes.fileIdx].string();
    res.shortFormId = refr->GetFormId() % 0x01000000;
  } else {
    res.shortFormId = refr->GetFormId();
  }

  for (int i = 0; i < 3; ++i) {
    res.pos[i] = refr->GetPos()[i];
    res.rot[i] = refr->GetAngle()[i];
  }

  res.worldOrCell = refr->GetCellOrWorld();

  res.inventoryDump.reset(
    new std::string(refr->GetInventory().ToJson().dump()));

  res.isHarvested = refr->IsHarvested();
  res.isOpen = refr->IsOpen();

  refr->GetRelootTime();

  auto relootMoment = refr->GetNextRelootMoment();
  if (relootMoment) {
    res.nextRelootDatetime =
      std::chrono::system_clock::to_time_t(*relootMoment);
  } else {
    res.nextRelootDatetime = 0;
  }

  auto actor = dynamic_cast<MpActor*>(refr);
  if (actor) {
    res.isRaceMenuOpen = actor->IsRaceMenuOpen();
    auto look = actor->GetLook();
    if (look)
      res.lookDump.reset(new std::string(look->ToJson()));
    res.equipmentDump.reset(new std::string(actor->GetEquipmentAsJson()));
  }

  return res;
}*/