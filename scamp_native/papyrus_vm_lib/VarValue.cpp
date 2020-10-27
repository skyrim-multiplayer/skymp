#include "Structures.h"

VarValue VarValue::CastToInt() const
{

  switch (this->type) {

    case kType_Object:
    case kType_Identifier:
      throw std::runtime_error("Wrong type in CastToInt");

    case kType_Integer:
      return VarValue((int32_t)this->data.i);

    case kType_Float:
      return VarValue((int32_t)this->data.f);

    case kType_Bool:
      return VarValue((int32_t)this->data.b);

    default:
      throw std::runtime_error("Wrong type in CastToInt");
  }
}

VarValue VarValue::CastToFloat() const
{

  switch (this->type) {

    case kType_Object:
    case kType_Identifier:
      throw std::runtime_error("Wrong type in CastToFloat");

    case kType_Integer:
      return VarValue((float)this->data.i);

    case kType_Float:
      return VarValue((float)this->data.f);

    case kType_Bool:
      return VarValue((float)this->data.b);

    default:
      throw std::runtime_error("Wrong type in CastToFloat");
  }
}

VarValue VarValue::CastToBool() const
{
  switch (this->type) {

    case kType_Object:
      if (this->data.id == nullptr)
        return VarValue((bool)false);
      else
        return VarValue((bool)true);

    case kType_Identifier:
      throw std::runtime_error("Wrong type in CastToBool");
    case kType_String: {
      std::string str;
      if (this->data.string == str)
        return VarValue((bool)false);
      else
        return VarValue((bool)true);
    }
    case kType_Integer:
      return VarValue((bool)this->data.i);

    case kType_Float:
      return VarValue((bool)this->data.f);

    case kType_Bool:
      return VarValue((bool)this->data.b);
    case kType_ObjectArray:
    case kType_StringArray:
    case kType_IntArray:
    case kType_FloatArray:
    case kType_BoolArray:
      return VarValue(this->pArray && this->pArray->size());
    default:
      throw std::runtime_error("Wrong type in CastToBool");
  }
}

VarValue::VarValue(uint8_t type)
{
  static std::string emptyLine;
  switch (type) {

    case kType_Object:
      this->type = this->kType_Object;
      this->data.id = nullptr;
      break;
    case kType_Identifier:
      this->type = this->kType_Identifier;
      this->data.string = nullptr;
      break;
    case kType_Integer:
      this->type = this->kType_Integer;
      this->data.i = 0;
      break;
    case kType_Float:
      this->type = this->kType_Float;
      this->data.f = 0.0f;
      break;
    case kType_Bool:
      this->type = this->kType_Bool;
      this->data.b = false;
      break;
    case kType_String:
      this->type = this->kType_String;
      this->data.string = emptyLine.c_str();
      break;
    case kType_ObjectArray:
    case kType_StringArray:
    case kType_IntArray:
    case kType_FloatArray:
    case kType_BoolArray:
      this->type = type;
      this->pArray = nullptr;
      break;

    default:
      throw std::runtime_error("Wrong type in VarValue::Constructor");
  }
}

VarValue::VarValue(uint8_t type, const char* value)
{
  this->type = this->kType_Identifier;
  this->data.string = value;
}

VarValue::VarValue(IGameObject* object)
{
  this->type = this->kType_Object;
  this->data.id = object;
}

VarValue::VarValue(int32_t value)
{
  this->type = this->kType_Integer;
  this->data.i = value;
}

VarValue::VarValue(const char* value)
{
  this->type = this->kType_String;
  this->data.string = value;
}

VarValue::VarValue(float value)
{
  this->type = this->kType_Float;
  this->data.f = value;
}

VarValue::VarValue(bool value)
{
  this->type = this->kType_Bool;
  this->data.b = value;
}

VarValue::VarValue(Viet::Promise<VarValue> promise)
{
  this->type = this->kType_Object;
  this->promise.reset(new Viet::Promise<VarValue>(promise));
}

VarValue VarValue::operator+(const VarValue& argument2)
{
  VarValue var;
  if (this->type == argument2.type) {

    switch (this->type) {

      case VarValue::kType_Integer:
        var.data.i = this->data.i + argument2.data.i;
        var.type = this->kType_Integer;
        return var;
      case VarValue::kType_Float:
        var.data.f = this->data.f + argument2.data.f;
        var.type = this->kType_Float;
        return var;
    }
  }
  throw std::runtime_error("Wrong type in operator+");
}

VarValue VarValue::operator-(const VarValue& argument2)
{
  VarValue var;
  if (this->type == argument2.type) {

    switch (this->type) {

      case VarValue::kType_Integer:
        var.data.i = this->data.i - argument2.data.i;
        var.type = this->kType_Integer;
        return var;
      case VarValue::kType_Float:
        var.data.f = this->data.f - argument2.data.f;
        var.type = this->kType_Float;
        return var;
    }
  }
  throw std::runtime_error("Wrong type in operator-");
}

VarValue VarValue::operator*(const VarValue& argument2)
{
  VarValue var;
  if (this->type == argument2.type) {

    switch (this->type) {
      case VarValue::kType_Integer:
        var.data.i = this->data.i * argument2.data.i;
        var.type = this->kType_Integer;
        return var;
      case VarValue::kType_Float:
        var.data.f = this->data.f * argument2.data.f;
        var.type = this->kType_Float;
        return var;
    }
  }
  throw std::runtime_error("Wrong type in operator*");
}

VarValue VarValue::operator/(const VarValue& argument2)
{
  VarValue var;
  if (this->type == argument2.type) {

    switch (this->type) {

      case VarValue::kType_Integer:
        var.data.i = 1;
        if (argument2.data.i != uint32_t(0) && this->data.i != uint32_t(0))
          var.data.i = this->data.i / argument2.data.i;
        var.type = this->kType_Integer;
        return var;
      case VarValue::kType_Float:
        var.data.f = 1.0f;
        if (argument2.data.f != float(0) && this->data.f != float(0))
          var.data.f = this->data.f / argument2.data.f;
        var.type = this->kType_Float;
        return var;
    }
  }
  throw std::runtime_error("Wrong type in operator/");
}

VarValue VarValue::operator%(const VarValue& argument2)
{
  VarValue var;
  if (this->type == argument2.type) {

    switch (this->type) {

      case VarValue::kType_Integer:
        var.data.i = 0;
        if (argument2.data.i != (uint32_t)0)
          var.data.i = this->data.i % argument2.data.i;
        var.type = this->kType_Integer;
        return var;
    }
  }
  throw std::runtime_error("Wrong type in operator%");
}

VarValue VarValue::operator!()
{
  VarValue var;

  switch (this->type) {

    case kType_Object:
      var.type = this->kType_Bool;
      var.data.b = (this->data.id == nullptr);
      return var;

    case kType_Identifier:
      throw std::runtime_error("Wrong type in operator!");

    case kType_Integer:
      var.type = this->kType_Bool;
      var.data.b = (this->data.i == 0);
      return var;

    case kType_Float:
      var.type = this->kType_Bool;
      var.data.b = (this->data.f == 0.0);
      return var;

    case kType_Bool:
      var.data.b = !this->data.b;
      var.type = this->kType_Bool;
      return var;

    case kType_String: {
      var.type = this->kType_Bool;
      static std::string emptyLine;
      var.data.b = (this->data.string == emptyLine);
      return var;
    }
    case kType_ObjectArray:
    case kType_StringArray:
    case kType_IntArray:
    case kType_FloatArray:
    case kType_BoolArray:
      var.type = this->kType_Bool;
      var.data.b = (this->pArray->size() < 1);
      return var;
    default:
      throw std::runtime_error("Wrong type in operator!");
  }
}

VarValue& VarValue::operator=(const VarValue& argument2)
{
  this->data = argument2.data;
  this->type = argument2.type;

  if (argument2.type >= argument2._ArraysStart &&
      argument2.type < argument2._ArraysEnd)
    this->pArray = argument2.pArray;

  return *this;
}

bool VarValue::operator==(const VarValue& argument2) const
{

  switch (this->type) {

    case VarValue::kType_Object: {
      IGameObject* id1 = nullptr;
      IGameObject* id2 = nullptr;

      if (this->data.id != nullptr)
        id1 = this->data.id;

      if (argument2.data.id != nullptr)
        id2 = argument2.data.id;

      return id1 == id2;
    }
    case VarValue::kType_Identifier:
    case VarValue::kType_String: {
      std::string s1;
      std::string s2;

      if (this->data.string != NULL)
        s1 = this->data.string;

      if (argument2.data.string != NULL)
        s2 = argument2.data.string;

      return s1 == s2;
    }
    case VarValue::kType_Integer:
      return this->CastToInt().data.i == argument2.CastToInt().data.i;
    case VarValue::kType_Float:
      return this->CastToFloat().data.f == argument2.CastToFloat().data.f;
    case VarValue::kType_Bool:
      return this->CastToBool().data.b == argument2.CastToBool().data.b;
  }
  throw std::runtime_error("Wrong type in operator!");
}

bool VarValue::operator!=(const VarValue& argument2) const
{
  return !(*this == argument2);
}

bool VarValue::operator>(const VarValue& argument2) const
{

  switch (this->type) {

    case VarValue::kType_Integer:
      return this->CastToInt().data.i > argument2.CastToInt().data.i;
    case VarValue::kType_Float:
      return this->CastToFloat().data.f > argument2.CastToFloat().data.f;
    case VarValue::kType_Bool:
      return this->CastToBool().data.b > argument2.CastToBool().data.b;
  }
  throw std::runtime_error("Wrong type in operator>");
}

bool VarValue::operator>=(const VarValue& argument2) const
{

  switch (this->type) {

    case VarValue::kType_Integer:
      return this->CastToInt().data.i >= argument2.CastToInt().data.i;
    case VarValue::kType_Float:
      return this->CastToFloat().data.f >= argument2.CastToFloat().data.f;
    case VarValue::kType_Bool:
      return this->CastToBool().data.b >= argument2.CastToBool().data.b;
  }
  throw std::runtime_error("Wrong type in operator>=");
}

bool VarValue::operator<(const VarValue& argument2) const
{

  switch (this->type) {

    case VarValue::kType_Integer:
      return this->CastToInt().data.i < argument2.CastToInt().data.i;
    case VarValue::kType_Float:
      return this->CastToFloat().data.f < argument2.CastToFloat().data.f;
    case VarValue::kType_Bool:
      return this->CastToBool().data.b < argument2.CastToBool().data.b;
  }
  throw std::runtime_error("Wrong type in operator<");
}

bool VarValue::operator<=(const VarValue& argument2) const
{

  switch (this->type) {

    case VarValue::kType_Integer:
      return this->CastToInt().data.i <= argument2.CastToInt().data.i;
    case VarValue::kType_Float:
      return this->CastToFloat().data.f <= argument2.CastToFloat().data.f;
    case VarValue::kType_Bool:
      return this->CastToBool().data.b <= argument2.CastToBool().data.b;
  }
  throw std::runtime_error("Wrong type in operator<=");
}

std::ostream& operator<<(std::ostream& os, const VarValue& varValue)
{
  switch (varValue.type) {
    case VarValue::kType_Object:
      os << "[Object '"
         << (varValue.data.id ? varValue.data.id->GetStringID() : "None")
         << "']";
      break;
    case VarValue::kType_Identifier:
      os << "[Identifier '" << varValue.data.string << "']";
      break;
    case VarValue::kType_String:
      os << "[String '" << varValue.data.string << "']";
      break;
    case VarValue::kType_Integer:
      os << "[Integer '" << varValue.data.i << "']";
      break;
    case VarValue::kType_Float:
      os << "[Float '" << varValue.data.f << "']";
      break;
    case VarValue::kType_Bool:
      os << "[Bool '" << varValue.data.b << "']";
      break;
    default:
      os << "[VarValue]";
      break;
  }
  return os;
}