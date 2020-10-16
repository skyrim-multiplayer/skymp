#include "SqliteChangeForm.h"
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

// https://stackoverflow.com/questions/9210528/split-string-with-delimiters-in-c
char* my_strsep(char** stringp, const char* delim)
{
  if (*stringp == NULL) {
    return NULL;
  }
  char* token_start = *stringp;
  *stringp = strpbrk(token_start, delim);
  if (*stringp) {
    **stringp = '\0';
    (*stringp)++;
  }
  return token_start;
}

constexpr auto g_sep = "|";
}

std::string SqliteChangeForm::GetJsonData() const
{
  return "v01:" + inv.ToJson().dump() + g_sep + lookDump + g_sep +
    equipmentDump;
}

void SqliteChangeForm::SetJsonData(const std::string& jsonData)
{
  static const auto versionLength = strlen("v01:");

  if (!memcmp(jsonData.data(), "v01:", versionLength)) {
    std::string myString = jsonData.data() + versionLength;
    char *token, *str;
    int doing = 0;

    str = myString.data();
    while ((token = my_strsep(&str, g_sep))) {
      switch (doing) {
        case 0:
          inv = DumpToStruct<Inventory>(token);
          break;
        case 1:
          lookDump = token;
          break;
        case 2:
          equipmentDump = token;
          break;
      }
      ++doing;
    }
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