#include "libespm/Utils.h"
#include "libespm/RecordHeaderAccess.h"
#include "libespm/ZlibUtils.h"
#include <map>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace espm::utils {

std::string ToString(GroupType type)
{
  switch (type) {
    case GroupType::TOP:
      return "TOP";
    case GroupType::WORLD_CHILDREN:
      return "WORLD_CHILDREN";
    case GroupType::INTERIOR_CELL_BLOCK:
      return "INTERIOR_CELL_BLOCK";
    case GroupType::INTERIOR_CELL_SUBBLOCK:
      return "INTERIOR_CELL_SUBBLOCK";
    case GroupType::EXTERIOR_CELL_BLOCK:
      return "EXTERIOR_CELL_BLOCK";
    case GroupType::EXTERIOR_CELL_SUBBLOCK:
      return "EXTERIOR_CELL_SUBBLOCK";
    case GroupType::CELL_CHILDREN:
      return "CELL_CHILDREN";
    case GroupType::TOPIC_CHILDREN:
      return "TOPIC_CHILDREN";
    case GroupType::CELL_PERSISTENT_CHILDREN:
      return "CELL_PERSISTENT_CHILDREN";
    case GroupType::CELL_TEMPORARY_CHILDREN:
      return "CELL_TEMPORARY_CHILDREN";
    case GroupType::CELL_VISIBLE_DISTANT_CHILDREN:
      return "CELL_VISIBLE_DISTANT_CHILDREN";
    default:
      throw std::runtime_error("unhandled case in ToString");
  }
}

bool IsItem(Type type) noexcept
{
  return type == "AMMO" || type == "ARMO" || type == "BOOK" ||
    type == "INGR" || type == "ALCH" || type == "SCRL" || type == "SLGM" ||
    type == "WEAP" || type == "MISC" ||
    type ==
    "LIGH"; // LIGH is not always an item but here we suppose it is a torch
}

uint32_t CalculateHashcode(const void* readBuffer, size_t length)
{
  return ZlibGetCRC32Checksum(readBuffer, length);
}

const std::map<std::string, uint32_t> kCorrectHashcode{
  { "Skyrim.esm", 0xaf75991dUL },
  { "Update.esm", 0x17ab5e20UL },
  { "Dawnguard.esm", 0xcc81e5d8UL },
  { "HearthFires.esm", 0xbad9393aUL },
  { "Dragonborn.esm", 0xeb10e82UL }
};

uint32_t GetCorrectHashcode(const std::string& fileName)
{
  auto iter = kCorrectHashcode.find(fileName);
  return iter == kCorrectHashcode.end() ? 0 : iter->second;
}

uint32_t GetMappedId(uint32_t id, const IdMapping& mapping) noexcept
{
  const uint32_t shortId = id % 0x01000000;
  const uint8_t index = id / 0x01000000;
  return shortId + (mapping[index] * 0x01000000);
}

std::wstring ReadWstring(const uint8_t* ptr)
{
  const uint16_t scriptNameSize = *reinterpret_cast<const uint16_t*>(ptr);
  const wchar_t* scriptName = reinterpret_cast<const wchar_t*>(ptr + 2);
  return std::wstring(scriptName, scriptNameSize / 2);
}

Property::Type GetElementType(Property::Type arrayType)
{
  return static_cast<Property::Type>(
    static_cast<std::underlying_type_t<Property::Type>>(arrayType) - 10);
}

const uint8_t* ReadPropertyValue(const uint8_t* p, Property* prop,
                                 uint16_t objFormat)
{
  const auto t = prop->type;
  switch (t) {
    case Property::Type::Object:
      if (objFormat == 1)
        prop->value.formId = *reinterpret_cast<const uint32_t*>(p);
      else if (objFormat == 2)
        prop->value.formId = *reinterpret_cast<const uint32_t*>(p + 4);
      else
        throw std::runtime_error("Unknown objFormat (" +
                                 std::to_string(objFormat) + ")");
      return p + 8;
    case Property::Type::Int:
      prop->value.integer = *reinterpret_cast<const int32_t*>(p);
      return p + 4;
    case Property::Type::Float:
      prop->value.floatingPoint = *reinterpret_cast<const float*>(p);
      return p + 4;
    case Property::Type::Bool:
      prop->value.boolean = *reinterpret_cast<const int8_t*>(p);
      return p + 1;
    case Property::Type::String: {
      uint16_t length = *reinterpret_cast<const uint16_t*>(p);
      p += 2;
      prop->value.str = { reinterpret_cast<const char*>(p), length };
      p += length;
      return p;
    }
    case Property::Type::ObjectArray:
    case Property::Type::IntArray:
    case Property::Type::FloatArray:
    case Property::Type::BoolArray:
    case Property::Type::StringArray: {
      uint32_t arrayLength = *reinterpret_cast<const uint32_t*>(p);
      p += 4;
      for (uint32_t i = 0; i < arrayLength; ++i) {
        Property element;
        element.type = GetElementType(t);
        p = ReadPropertyValue(p, &element, objFormat);
        prop->array.push_back(element.value);
      }
      return p;
    }
    default:
      throw std::runtime_error(
        "Script properties with type " +
        std::to_string(
          static_cast<std::underlying_type_t<Property::Type>>(t)) +
        " are not yet supported");
  }
}

void FillScriptArray(const uint8_t* p, std::vector<Script>& out,
                     uint16_t objFormat)
{
  for (uint16_t i = 0; i < out.size(); ++i) {
    const uint16_t length = *reinterpret_cast<const uint16_t*>(p);
    p += 2;
    out[i].scriptName = std::string(reinterpret_cast<const char*>(p), length);
    p += length;
    out[i].status = *p;
    p++;
    const uint16_t numProperties = *reinterpret_cast<const uint16_t*>(p);
    p += 2;
    for (uint16_t j = 0; j < numProperties; ++j) {
      const uint16_t length = *reinterpret_cast<const uint16_t*>(p);
      p += 2;

      Property prop;
      prop.name = std::string(reinterpret_cast<const char*>(p), length);
      p += length;
      prop.type = static_cast<Property::Type>(*p);
      p++;
      prop.status = *p;
      p++;
      p = ReadPropertyValue(p, &prop, objFormat);
      out[i].properties.insert(prop);
    }
  }
}

uint64_t MakeUInt64(uint32_t high, uint32_t low)
{
  return (static_cast<uint64_t>(high) << 32) | static_cast<uint64_t>(low);
}

std::vector<CONT::ContainerObject> GetContainerObjects(
  const RecordHeader* rec, CompressedFieldsCache& compressedFieldsCache)
{
  std::vector<CONT::ContainerObject> objects;
  RecordHeaderAccess::IterateFields(
    rec,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "CNTO", 4)) {
        objects.push_back(
          *reinterpret_cast<const CONT::ContainerObject*>(data));
      } else if (!std::memcmp(type, "COED", 4)) {
        // Not supported
      }
    },
    compressedFieldsCache);
  return objects;
}
}
