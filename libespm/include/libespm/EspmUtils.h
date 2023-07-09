#pragma once
#include "GroupType.h"
#include "IdMapping.h"
#include "Property.h"
#include "Script.h"
#include "Type.h"
#include "libespm/CONT.h"
#include <array>
#include <string>
#include <vector>

namespace espm::utils {

std::string ToString(GroupType type);
bool IsItem(Type type) noexcept;
uint32_t CalculateHashcode(const void* readBuffer, size_t length);
uint32_t GetCorrectHashcode(const std::string& fileName);
uint32_t GetMappedId(uint32_t id, const IdMapping& mapping) noexcept;
std::wstring ReadWstring(const uint8_t* ptr);
Property::Type GetElementType(Property::Type arrayType);
const uint8_t* ReadPropertyValue(const uint8_t* p, Property* prop,
                                 uint16_t objFormat);
void FillScriptArray(const uint8_t* p, std::vector<Script>& out,
                     uint16_t objFormat);
uint64_t MakeUInt64(uint32_t high, uint32_t low);
std::vector<CONT::ContainerObject> GetContainerObjects(
  const RecordHeader* rec, CompressedFieldsCache& compressedFieldsCache);

}
