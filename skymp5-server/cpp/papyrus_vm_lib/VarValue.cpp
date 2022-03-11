#include "Structures.h"
#include "VirtualMachine.h"

#include <cmath>
#include <sstream>

VarValue VarValue::CastToInt() const
{
  switch (type) {
  case Type::String: {
   return VarValue(atoi(this->data.string));
  }
  case Type::Integer:
      return VarValue(data.i);
  case Type::Float:
      return VarValue(static_cast<int32_t>(data.f));
  case Type::Bool:
      return VarValue(static_cast<int32_t>(data.b));
  default:
      throw std::runtime_error("Wrong type in CastToInt");
  }
}

VarValue VarValue::CastToFloat() const
{
  switch (type) {
    case Type::String: {
      return VarValue(strtod(data.string, nullptr));
    }
    case Type::Integer:
      return VarValue(static_cast<double>(this->data.i));
    case Type::Float:
      return VarValue(this->data.f);
    case Type::Bool:
      return VarValue(static_cast<double>(this->data.b));
    default:
      throw std::runtime_error("Wrong type in CastToFloat");
  }
}

VarValue VarValue::CastToBool() const
{
  switch (type) {
    case Type::Object:
        return data.id == nullptr ? VarValue(false) : VarValue(true);
    case Type::Identifier:
      throw std::runtime_error("Wrong type in CastToBool");
    case Type::String: {
       return VarValue(std::strlen(data.string) > 0);
    }
    case Type::Integer:
      return VarValue(static_cast<bool>(data.i));
    case Type::Float:
      return VarValue(static_cast<bool>(data.f));
    case Type::Bool:
      return VarValue(data.b);
    case Type::ObjectArray:
    case Type::StringArray:
    case Type::IntArray:
    case Type::FloatArray:
    case Type::BoolArray:
       return VarValue(pArray && !pArray->empty());
    default:
      throw std::runtime_error("Wrong type in CastToBool");
  }
}

void VarValue::Then(std::function<void(VarValue)> cb) const
{
  if (!promise) {
    throw std::runtime_error("Not a promise");
  }
  promise->Then(cb);
}

namespace  {
     constexpr inline std::string_view EMPTY_STR = "";
}

VarValue::VarValue(const Type type_) : data(nullptr), type(type_)
{
  switch (type) {
    case Type::Object:
    case Type::Identifier:
      return;
    case Type::Integer:
      data.i = 0;
      return;
    case Type::Float:
      data.f = 0.0;
      return;
    case Type::Bool:
      data.b = false;
      return;
    case Type::String:
      data.string = EMPTY_STR.data();
      return;
    case Type::ObjectArray:
    case Type::StringArray:
    case Type::IntArray:
    case Type::FloatArray:
    case Type::BoolArray:
      this->pArray = nullptr;
      return;
    default:
        break;
  }

  throw std::runtime_error("Wrong type in VarValue::Constructor");
}

VarValue::VarValue(IGameObject* object) : data(object), type(Type::Object)
{}

VarValue::VarValue(const int32_t value) : data(value), type(Type::Integer)
{}

VarValue::VarValue(const char* value, const Type type_) : data(value), type(type_)
{
   if(type_ != Type::String && type_ != Type::Identifier)
    throw std::runtime_error("Wrong type in VarValue::Constructor VarValue(const char* value, const Type type_)");
}

VarValue::VarValue(const std::string& value)
{
  this->type = Type::String;
  this->stringHolder.reset(new std::string(value));
  this->data.string = this->stringHolder->data();
}

VarValue::VarValue(const double value) : data(value), type(Type::Float)
{}

VarValue::VarValue(const bool value) : data(value), type(Type::Bool)
{}

VarValue::VarValue(const Viet::Promise<VarValue>& promise)
{
  this->type = Type::Object;
  this->data.id = nullptr;
  this->promise.reset(new Viet::Promise(promise));
}

VarValue::VarValue(const std::shared_ptr<IGameObject>& object)
  : VarValue(object.get())
{
  owningObject = object;
}

int32_t VarValue::GetMetaStackId() const
{
  if (stackId < 0) {
    throw std::runtime_error("This VarValue has no metadata");
  }
  return stackId;
}

void VarValue::SetMetaStackIdHolder(const std::shared_ptr<StackIdHolder>& stackIdHolder_)
{
  stackId = stackIdHolder_->GetStackId();
}

VarValue VarValue::AttachTestStackId(VarValue original, int32_t stackId)
{
  original.stackId = stackId;
  return original;
}

namespace
{

  bool IsNumber(const VarValue& v)
  {
    return v.GetType() == VarValue::Type::Integer || v.GetType() == VarValue::Type::Float;
  }

  double ToDouble(const VarValue& v)
  {
    switch (v.GetType()) {
      case VarValue::Type::Integer:
      case VarValue::Type::Float:
        return static_cast<int32_t>(v);
      default:
         break;  
    }

    throw std::runtime_error("Wrong type in ToDouble");
  }

  VarValue ConstructArithmeticResult(const VarValue& op1, const VarValue& op2, const double res)
  {
    if (op1.GetType() == VarValue::Type::Float || op2.GetType() == VarValue::Type::Float) {
       return VarValue(res);
    }
    return VarValue(static_cast<int32_t>(floor(res)));
  }

} //Anonymous namespace End

VarValue VarValue::operator+(const VarValue& argument2) const
{
  if (type == argument2.type) {
    switch (type) {
    case Type::Integer:
        return VarValue(data.i + argument2.data.i);
    case Type::Float:
         return VarValue(data.f + argument2.data.f);
      default:
        break;
    }
  }

  if (IsNumber(*this) && IsNumber(argument2)) {
    return ConstructArithmeticResult(*this, argument2,
                                     ToDouble(*this) + ToDouble(argument2));
  }

  throw std::runtime_error("Wrong type in operator+");
}

VarValue VarValue::operator-(const VarValue& argument2) const
{
  if (type == argument2.type) {
    switch (type) {
      case Type::Integer:
        return VarValue(data.i - argument2.data.i);
      case Type::Float:
        return VarValue(data.f - argument2.data.f);
      default:
        break;
    }
  }

  if (IsNumber(*this) && IsNumber(argument2)) {
    return ConstructArithmeticResult(*this, argument2,
                                     ToDouble(*this) - ToDouble(argument2));
  }

  throw std::runtime_error("Wrong type in operator-");
}

VarValue VarValue::operator*(const VarValue& argument2) const
{
  if (type == argument2.type) {
    switch (type) {
      case Type::Integer:
         return VarValue(data.i * argument2.data.i);
      case Type::Float:
        return VarValue(data.f * argument2.data.f);
      default:
        break;
    }
  }

  if (IsNumber(*this) && IsNumber(argument2)) {
    return ConstructArithmeticResult(*this, argument2,
                                     ToDouble(*this) * ToDouble(argument2));
  }

  throw std::runtime_error("Wrong type in operator*");
}

VarValue VarValue::operator/(const VarValue& argument2) const
{
  if (type == argument2.type) {
    switch (type) {
      case Type::Integer: 
        if (argument2.data.i == 0 || this->data.i == 0)
        {
          return VarValue(1);
        }

        return VarValue(data.i / argument2.data.i);
      case Type::Float:
        if (argument2.data.f == 0.0)
        {
          return VarValue(1.0);
        }

        return VarValue(data.f / argument2.data.f);
      default:
        break;
    }
  }

  if (IsNumber(*this) && IsNumber(argument2)) {
    return ConstructArithmeticResult(*this, argument2,
                                     ToDouble(*this) / ToDouble(argument2));
  }

  throw std::runtime_error("Wrong type in operator/");
}

VarValue VarValue::operator%(const VarValue& argument2) const
{
  if (type != argument2.type)
  {
    throw std::runtime_error("Wrong type in operator%");
  }

  switch (type) {
    case Type::Integer:
      if (argument2.data.i == 0)
      {
        return VarValue(0);
      }
        return VarValue(this->data.i % argument2.data.i);
    default:
      break;
    }

   throw std::runtime_error("Wrong type in operator%");
}

VarValue VarValue::operator!() const
{
  switch (type) {
    case Type::Object:
      return VarValue(data.id == nullptr);
    case Type::Identifier:
      throw std::runtime_error("Wrong type in operator!");
    case Type::Integer:
       return VarValue(data.i == 0);
    case Type::Float:
      return VarValue(data.f == 0.0);
    case Type::Bool:
      return VarValue(!data.b);
    case Type::String: {
      return VarValue(std::strlen(data.string) == 0);
    }
    case Type::ObjectArray:
    case Type::StringArray:
    case Type::IntArray:
    case Type::FloatArray:
    case Type::BoolArray:
      return VarValue(pArray->empty());
    default:
      break;
  }

  throw std::runtime_error("Wrong type in operator!");
}

bool VarValue::operator==(const VarValue& argument2) const
{
  switch (type) {
    case Type::Object: {
      return argument2.type == Type::Object &&
        (argument2.data.id == data.id ||
         argument2.data.id && data.id &&
         data.id->EqualsByValue(*argument2.data.id));
    }
    case Type::Identifier:
    case Type::String: {

      if (argument2.type != Type::String) {
        return false;
      }

      return std::string_view(data.string) == std::string_view(argument2.data.string);
    }
    case Type::Float:
      return std::fabs(CastToFloat().data.f - argument2.CastToFloat().data.f) < EPSILON;
    case Type::Integer:
      return CastToInt().data.i == argument2.CastToInt().data.i;
    case Type::Bool:
      return CastToBool().data.b == argument2.CastToBool().data.b;
    default:
      break;
  }
  throw std::runtime_error("Wrong type in operator!");
}

bool VarValue::operator!=(const VarValue& argument2) const
{
  return !(*this == argument2);
}

bool VarValue::operator>(const VarValue& argument2) const
{
  switch (type) {
    case Type::Integer:
      return CastToInt().data.i > argument2.CastToInt().data.i;
    case Type::Float:
      return CastToFloat().data.f > argument2.CastToFloat().data.f;
    case Type::Bool:
      return CastToBool().data.b > argument2.CastToBool().data.b;
    default:
      break;
  }
  throw std::runtime_error("Wrong type in operator>");
}

bool VarValue::operator>=(const VarValue& argument2) const
{
  switch (type) {
    case Type::Integer:
      return this->CastToInt().data.i >= argument2.CastToInt().data.i;
    case Type::Float:
      return this->CastToFloat().data.f >= argument2.CastToFloat().data.f;
    case Type::Bool:
      return this->CastToBool().data.b >= argument2.CastToBool().data.b;
    default:
      break;
  }
  throw std::runtime_error("Wrong type in operator>=");
}

bool VarValue::operator<(const VarValue& argument2) const
{
  switch (type) {
    case Type::Integer:
      return CastToInt().data.i < argument2.CastToInt().data.i;
    case Type::Float:
      return CastToFloat().data.f < argument2.CastToFloat().data.f;
    case Type::Bool:
      return CastToBool().data.b < argument2.CastToBool().data.b;
    default:
      break;
  }
  throw std::runtime_error("Wrong type in operator<");
}

bool VarValue::operator<=(const VarValue& argument2) const
{
  switch (type) {
    case Type::Integer:
      return CastToInt().data.i <= argument2.CastToInt().data.i;
    case Type::Float:
      return CastToFloat().data.f <= argument2.CastToFloat().data.f;
    case Type::Bool:
      return CastToBool().data.b <= argument2.CastToBool().data.b;
    default:
      break;
  }
  throw std::runtime_error("Wrong type in operator<=");
}

std::ostream& operator<<(std::ostream& os, const VarValue& varValue)
{
  switch (varValue.type) {
    case VarValue::Type::Object:
      os << "[Object '"
         << (varValue.data.id ? varValue.data.id->GetStringID() : "None")
         << "']";
      break;
    case VarValue::Type::Identifier:
      os << "[Identifier '" << varValue.data.string << "']";
      break;
    case VarValue::Type::String:
      os << "[String '" << varValue.data.string << "']";
      break;
    case VarValue::Type::Integer:
      os << "[Integer '" << varValue.data.i << "']";
      break;
    case VarValue::Type::Float:
      os << "[Float '" << varValue.data.f << "']";
      break;
    case VarValue::Type::Bool:
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
  //Fix bugprone-unhandled-self-assignment
  if(this == &arg2) {
    return *this;
  }
      
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
  if (arg2.IsArray()) {
    pArray = arg2.pArray;
  }

  promise = arg2.promise;

  if (arg2.stringHolder) {
    stringHolder.reset(new std::string(*arg2.stringHolder));
    data.string = stringHolder->data();
  } else {
    stringHolder.reset();
  }

  return *this;
}
