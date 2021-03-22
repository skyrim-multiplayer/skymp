#include "Structures.h"
#include "VirtualMachine.h"

#include <cmath>
#include <sstream>

VarValue VarValue::CastToInt() const
{
  switch (this->type) {
    case kType_String:
      return VarValue((int32_t)atoi(this->data.string));
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
    case kType_String: {
      std::string str = static_cast<const char*>(*this);
      std::istringstream ss(str);
      double d = 0.0;
      ss >> d;
      return VarValue(d);
    }
    case kType_Integer:
      return VarValue(static_cast<double>(this->data.i));
    case kType_Float:
      return VarValue(static_cast<double>(this->data.f));
    case kType_Bool:
      return VarValue(static_cast<double>(this->data.b));
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

void VarValue::Then(std::function<void(VarValue)> cb)
{
  if (!promise)
    throw std::runtime_error("Not a promise");
  promise->Then(cb);
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
      this->type = static_cast<Type>(type);
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

VarValue::VarValue(const std::string& value)
{
  this->type = this->kType_String;
  this->stringHolder.reset(new std::string(value));
  this->data.string = this->stringHolder->data();
}

VarValue::VarValue(double value)
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
  this->data.id = nullptr;
  this->promise.reset(new Viet::Promise<VarValue>(promise));
}

VarValue::VarValue(IGameObject::Ptr object)
  : VarValue(object.get())
{
  owningObject = object;
}

int32_t VarValue::GetMetaStackId() const
{
  if (stackId < 0)
    throw std::runtime_error("This VarValue has no metadata");
  return stackId;
}

void VarValue::SetMetaStackIdHolder(
  std::shared_ptr<StackIdHolder> stackIdHolder_)
{
  stackId = stackIdHolder_->GetStackId();
}

VarValue VarValue::AttachTestStackId(VarValue original, int32_t stackId)
{
  original.stackId = stackId;
  return original;
}

namespace {
inline bool IsNumber(const VarValue& v)
{
  return v.GetType() == VarValue::kType_Integer ||
    v.GetType() == VarValue::kType_Float;
}

inline double ToDouble(const VarValue& v)
{
  switch (v.GetType()) {
    case VarValue::kType_Integer:
      return static_cast<double>(static_cast<int32_t>(v));
    case VarValue::kType_Float:
      return static_cast<double>(static_cast<int32_t>(v));
  }
  throw std::runtime_error("Wrong type in ToDouble");
}

inline VarValue ConstructArithmeticResult(const VarValue& op1,
                                          const VarValue& op2, double res)
{
  if (op1.GetType() == VarValue::kType_Float ||
      op2.GetType() == VarValue::kType_Float) {
    return VarValue(static_cast<float>(res));
  }
  return VarValue(static_cast<int32_t>(floor(res)));
}
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

  if (IsNumber(*this) && IsNumber(argument2)) {
    return ConstructArithmeticResult(*this, argument2,
                                     ToDouble(*this) + ToDouble(argument2));
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

  if (IsNumber(*this) && IsNumber(argument2)) {
    return ConstructArithmeticResult(*this, argument2,
                                     ToDouble(*this) - ToDouble(argument2));
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

  if (IsNumber(*this) && IsNumber(argument2)) {
    return ConstructArithmeticResult(*this, argument2,
                                     ToDouble(*this) * ToDouble(argument2));
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
        if (argument2.data.f != float(0))
          var.data.f = this->data.f / argument2.data.f;
        var.type = this->kType_Float;
        return var;
    }
  }

  if (IsNumber(*this) && IsNumber(argument2)) {
    return ConstructArithmeticResult(*this, argument2,
                                     ToDouble(*this) / ToDouble(argument2));
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

bool VarValue::operator==(const VarValue& argument2) const
{

  switch (this->type) {

    case VarValue::kType_Object: {
      return argument2.type == VarValue::kType_Object &&
        (argument2.data.id == data.id ||
         (argument2.data.id && data.id &&
          data.id->EqualsByValue(*argument2.data.id)));
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

VarValue& VarValue::operator=(const VarValue& arg2)
{
  // DO NOT DO THIS:
  /// objectType = arg2.objectType;

  // Object dynamic cast is relying on the current implementation (See first
  // argument in CastObjectToObject). Once you try to remove this operator
  // overload or copy 'objectType', you would run into issues with casting
  // between Papyrus object types.

  // At the moment when this comment has been written,
  // there was no unit test able to reproduce it.Good luck with debugging.

  data = arg2.data;
  type = arg2.type;
  owningObject = arg2.owningObject;

  // TODO: Is this check actually needed?
  if (arg2.type >= arg2._ArraysStart && arg2.type < arg2._ArraysEnd)
    pArray = arg2.pArray;

  promise = arg2.promise;

  if (arg2.stringHolder) {
    stringHolder.reset(new std::string(*arg2.stringHolder));
    data.string = stringHolder->data();
  } else {
    stringHolder.reset();
  }

  return *this;
}