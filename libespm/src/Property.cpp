#include "libespm/Property.h"
#include <ostream>

namespace espm {

Property Property::Object(std::string propertyName, uint32_t formId)
{
  Property res{ propertyName, Type::Object };
  res.value.formId = formId;
  return res;
}

Property Property::Int(std::string propertyName, int32_t integer)
{
  Property res{ propertyName, Type::Int };
  res.value.integer = integer;
  return res;
}

Property Property::Bool(std::string propertyName, bool boolean)
{
  Property res{ propertyName, Type::Bool };
  res.value.boolean = boolean ? 1 : 0;
  return res;
}

Property Property::Float(std::string propertyName, float floatingPoint)
{
  Property res{ propertyName, Type::Float };
  res.value.floatingPoint = floatingPoint;
  return res;
}

std::ostream& operator<<(std::ostream& os, const Property& prop)
{
  os << "[" << prop.name;
  switch (prop.type) {
    case Property::Type::Bool:
      os << "=" << (prop.value.boolean ? "true" : "false");
      break;
    case Property::Type::Int:
      os << "=" << prop.value.integer;
      break;
    case Property::Type::Float:
      os << "=" << prop.value.floatingPoint;
      break;
    case Property::Type::Object:
      os << "=FormID_" << std::hex << prop.value.formId << std::dec;
      break;
    default:
      os << "=...";
      break;
  }
  os << ", status=" << static_cast<int>(prop.status) << "]";
  return os;
}

bool Property::operator==(const Property& rhs) const
{
  return ToTuple() == rhs.ToTuple();
}

bool Property::operator!=(const Property& rhs) const
{
  return !(*this == rhs);
}

bool Property::operator<(const Property& rhs) const
{
  return ToTuple() < rhs.ToTuple();
}

}
