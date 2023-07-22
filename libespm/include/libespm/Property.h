#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

struct Property
{
  enum class Type : uint32_t
  {
    Invalid = 0,
    Object = 1,
    String = 2,
    Int = 3,
    Float = 4,
    Bool = 5,
    ObjectArray = 11,
    StringArray = 12,
    IntArray = 13,
    FloatArray = 14,
    BoolArray = 15
  };

  enum class Status : uint8_t
  {
    Edited = 1,
    Removed = 3
  };

  union Value
  {
    uint32_t formId;
    int32_t integer;
    int8_t boolean;
    float floatingPoint;

    struct Str
    {
      const char* data;
      size_t length;
    } str = { 0, 0 };
  };

  static Property Object(std::string propertyName, uint32_t formId);
  static Property Int(std::string propertyName, int32_t integer);
  static Property Bool(std::string propertyName, bool boolean);
  static Property Float(std::string propertyName, float floatingPoint);

  auto ToTuple() const
  {
    return std::make_tuple(name, type, value.str.data, value.str.length,
                           status);
  }

  bool operator==(const Property& rhs) const;
  bool operator!=(const Property& rhs) const;
  bool operator<(const Property& rhs) const;
  friend std::ostream& operator<<(std::ostream& os, const Property& prop);

  std::string name;
  Type type = Type::Invalid;
  Value value;
  std::vector<Value> array;
  uint8_t status = static_cast<std::underlying_type_t<Status>>(Status::Edited);
};

}

#pragma pack(pop)
