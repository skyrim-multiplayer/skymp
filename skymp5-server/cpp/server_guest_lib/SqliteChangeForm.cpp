#include "SqliteChangeForm.h"
#include "JsonUtils.h"
#include "MpActor.h"
#include "WorldState.h"
#include <ctime>
#include <simdjson.h>
#include <string>

std::string SqliteChangeForm::GetJsonData() const
{
  auto j =
    nlohmann::json::object({ { "inv", inv.ToJson() },
                             { "lookDump", lookDump },
                             { "equipmentDump", equipmentDump },
                             { "isDisabled", isDisabled },
                             { "profileId", profileId },
                             { "dynamicFields", dynamicFields.GetAsJson() } });
  return "v01:" + j.dump();
}

void SqliteChangeForm::SetJsonData(const std::string& jsonData)
{
  static const JsonPointer inv("inv"), lookDump("lookDump"),
    equipmentDump("equipmentDump"), isDisabled("isDisabled"),
    profileId("profileId"), dynamicFields("dynamicFields");

  static const auto versionLength = strlen("v01:");

  if (!memcmp(jsonData.data(), "v01:", versionLength)) {
    std::string myString = jsonData.data() + versionLength;

    simdjson::dom::parser p;

    auto j = p.parse(myString).value();
    {
      simdjson::dom::element jInv;
      ReadEx(j, inv, &jInv);
      this->inv = Inventory::FromJson(jInv);
    }
    {
      const char* jLookDump;
      ReadEx(j, lookDump, &jLookDump);
      this->lookDump = jLookDump;
    }
    {
      const char* jEquipmentDump;
      ReadEx(j, equipmentDump, &jEquipmentDump);
      this->equipmentDump = jEquipmentDump;
    }
    ReadEx(j, isDisabled, &this->isDisabled);
    ReadEx(j, profileId, &this->profileId);

    simdjson::dom::element dynamicFieldsValue;
    ReadEx(j, dynamicFields, &dynamicFieldsValue);
    this->dynamicFields = DynamicFields::FromJson(nlohmann::json::parse(
      static_cast<std::string>(simdjson::minify(dynamicFieldsValue))));

  } else {
    std::stringstream ss;
    ss << "Bad jsonData version: '" << jsonData << "'";
    throw std::runtime_error(ss.str());
  }
}

std::string SqliteChangeForm::GetFormDesc() const
{
  return formDesc.ToString();
}

void SqliteChangeForm::SetFormDesc(const std::string& newFormDesc)
{
  formDesc = FormDesc::FromString(newFormDesc);
}

std::string SqliteChangeForm::GetBaseFormDesc() const
{
  return baseDesc.ToString();
}

void SqliteChangeForm::SetBaseFormDesc(const std::string& newFormDesc)
{
  baseDesc = FormDesc::FromString(newFormDesc);
}

float SqliteChangeForm::GetX() const
{
  return position.x;
}

void SqliteChangeForm::SetX(float v)
{
  position.x = v;
}

float SqliteChangeForm::GetY() const
{
  return position.y;
}

void SqliteChangeForm::SetY(float v)
{
  position.y = v;
}

float SqliteChangeForm::GetZ() const
{
  return position.z;
}

void SqliteChangeForm::SetZ(float v)
{
  position.z = v;
}

float SqliteChangeForm::GetAngleX() const
{
  return angle.x;
}

void SqliteChangeForm::SetAngleX(float v)
{
  angle.x = v;
}

float SqliteChangeForm::GetAngleY() const
{
  return angle.y;
}

void SqliteChangeForm::SetAngleY(float v)
{
  angle.y = v;
}

float SqliteChangeForm::GetAngleZ() const
{
  return angle.z;
}

void SqliteChangeForm::SetAngleZ(float v)
{
  angle.z = v;
}
