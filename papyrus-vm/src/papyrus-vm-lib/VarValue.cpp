#include "antigo/Context.h"
#include "papyrus-vm/Structures.h"
#include "papyrus-vm/VirtualMachine.h"

#include <cmath>
#include <spdlog/spdlog.h>
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
      spdlog::error("VarValue::CastToInt - Wrong type in CastToInt");
      return VarValue((int32_t)0);
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
      spdlog::error("VarValue::CastToFloat - Wrong type in CastToFloat");
      return VarValue(static_cast<double>(0));
  }
}

VarValue VarValue::CastToBool() const
{
  switch (this->type) {
    case kType_Object:
      if (this->data.id == nullptr) {
        return VarValue(false);
      } else {
        return VarValue(true);
      }
    case kType_Identifier:
      spdlog::error("VarValue::CastToBool - Wrong type in CastToBool");
      return VarValue(false);
    case kType_String: {
      std::string str;
      if (this->data.string == str) {
        return VarValue(false);
      } else {
        return VarValue(true);
      }
    }
    case kType_Integer:
      return VarValue(static_cast<bool>(this->data.i));
    case kType_Float:
      return VarValue(static_cast<bool>(this->data.f));
    case kType_Bool:
      return VarValue(static_cast<bool>(this->data.b));
    case kType_ObjectArray:
    case kType_StringArray:
    case kType_IntArray:
    case kType_FloatArray:
    case kType_BoolArray:
      return VarValue(this->pArray && this->pArray->size() > 0);
    default:
      spdlog::error("VarValue::CastToBool - Wrong type in CastToBool");
      return VarValue(false);
  }
}

void VarValue::Then(std::function<void(VarValue)> cb)
{
  if (!promise) {
    throw std::runtime_error("Not a promise");
  }
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
      this->type = this->kType_Object;
      this->data.id = nullptr;
      spdlog::error("VarValue::VarValue - Unknown type passed {}",
                    static_cast<int>(type));
      break;
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

VarValue::VarValue(std::shared_ptr<IGameObject> object)
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

void VarValue::SetMetaStackIdHolder(const StackIdHolder& stackIdHolder_)
{
  stackId = stackIdHolder_.GetStackId();
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
  spdlog::error("::ToDouble - Wrong type passed {}",
                static_cast<int>(v.GetType()));
  return 0.0;
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
      default:
        break;
    }
  }

  if (IsNumber(*this) && IsNumber(argument2)) {
    return ConstructArithmeticResult(*this, argument2,
                                     ToDouble(*this) + ToDouble(argument2));
  }

  spdlog::error("VarValue::operator+ - Wrong type");
  return VarValue(0.0);
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
      default:
        break;
    }
  }

  if (IsNumber(*this) && IsNumber(argument2)) {
    return ConstructArithmeticResult(*this, argument2,
                                     ToDouble(*this) - ToDouble(argument2));
  }

  spdlog::error("VarValue::operator- - Wrong type");
  return VarValue(0.0);
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
      default:
        break;
    }
  }

  if (IsNumber(*this) && IsNumber(argument2)) {
    return ConstructArithmeticResult(*this, argument2,
                                     ToDouble(*this) * ToDouble(argument2));
  }

  spdlog::error("VarValue::operator* - Wrong type");
  return VarValue(0.0);
}

VarValue VarValue::operator/(const VarValue& argument2)
{
  VarValue var;
  if (this->type == argument2.type) {
    switch (this->type) {
      case VarValue::kType_Integer:
        var.data.i = 1;
        if (argument2.data.i != 0 && this->data.i != 0) {
          var.data.i = this->data.i / argument2.data.i;
        }
        var.type = this->kType_Integer;
        return var;
      case VarValue::kType_Float:
        var.data.f = 1.0f;
        if (argument2.data.f != 0.f) {
          var.data.f = this->data.f / argument2.data.f;
        }
        var.type = this->kType_Float;
        return var;
      default:
        break;
    }
  }

  if (IsNumber(*this) && IsNumber(argument2)) {
    return ConstructArithmeticResult(*this, argument2,
                                     ToDouble(*this) / ToDouble(argument2));
  }

  spdlog::error("VarValue::operator/ - Wrong type");
  return VarValue(0.0);
}

VarValue VarValue::operator%(const VarValue& argument2)
{
  VarValue var;
  if (this->type == argument2.type) {
    switch (this->type) {
      case VarValue::kType_Integer:
        var.data.i = 0;
        if (argument2.data.i != 0) {
          var.data.i = this->data.i % argument2.data.i;
        }
        var.type = this->kType_Integer;
        return var;
      default:
        break;
    }
  }
  spdlog::error("VarValue::operator% - Wrong type");
  return VarValue(0);
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
      spdlog::error("VarValue::operator! - Wrong type");
      var.type = this->kType_Bool;
      var.data.b = false;
      return var;
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
      static const std::string g_emptyLine;
      var.data.b = (this->data.string == g_emptyLine);
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
      spdlog::error("VarValue::operator! - Wrong type");
      var.type = this->kType_Bool;
      var.data.b = false;
      return var;
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
      if (argument2.type != VarValue::kType_String) {
        return false;
      }

      std::string s1;
      std::string s2;

      if (this->data.string != nullptr) {
        s1 = this->data.string;
      }

      if (argument2.data.string != nullptr) {
        s2 = argument2.data.string;
      }

      return s1 == s2;
    }
    case VarValue::kType_Integer:
      return this->CastToInt().data.i == argument2.CastToInt().data.i;
    case VarValue::kType_Float:
      return this->CastToFloat().data.f == argument2.CastToFloat().data.f;
    case VarValue::kType_Bool:
      return this->CastToBool().data.b == argument2.CastToBool().data.b;
    default:
      break;
  }
  spdlog::error("VarValue::operator== - Wrong type");
  return false;
}

bool VarValue::operator!=(const VarValue& argument2) const
{
  return !(*this == argument2);
}

bool VarValue::operator>(const VarValue& argument2) const
{
  ANTIGO_CONTEXT_INIT(ctx);
  ctx.AddMessage("next: type left, type right");
  ctx.AddUnsigned(GetType());
  ctx.AddUnsigned(argument2.GetType());

  switch (this->type) {
    case VarValue::kType_Integer:
      return this->CastToInt().data.i > argument2.CastToInt().data.i;
    case VarValue::kType_Float:
      return this->CastToFloat().data.f > argument2.CastToFloat().data.f;
    case VarValue::kType_Bool:
      return this->CastToBool().data.b > argument2.CastToBool().data.b;
    default:
      break;
  }
  spdlog::error("VarValue::operator> - Wrong type");
  return false;
}

bool VarValue::operator>=(const VarValue& argument2) const
{
  ANTIGO_CONTEXT_INIT(ctx);
  ctx.AddMessage("next: type left, type right");
  ctx.AddUnsigned(GetType());
  ctx.AddUnsigned(argument2.GetType());

  switch (this->type) {
    case VarValue::kType_Integer:
      return this->CastToInt().data.i >= argument2.CastToInt().data.i;
    case VarValue::kType_Float:
      return this->CastToFloat().data.f >= argument2.CastToFloat().data.f;
    case VarValue::kType_Bool:
      return this->CastToBool().data.b >= argument2.CastToBool().data.b;
    default:
      break;
  }
  spdlog::error("VarValue::operator>= - Wrong type");
  return false;
}

bool VarValue::operator<(const VarValue& argument2) const
{
  ANTIGO_CONTEXT_INIT(ctx);
  ctx.AddMessage("next: type left, type right");
  ctx.AddUnsigned(GetType());
  ctx.AddUnsigned(argument2.GetType());

  switch (this->type) {
    case VarValue::kType_Integer:
      return this->CastToInt().data.i < argument2.CastToInt().data.i;
    case VarValue::kType_Float:
      return this->CastToFloat().data.f < argument2.CastToFloat().data.f;
    case VarValue::kType_Bool:
      return this->CastToBool().data.b < argument2.CastToBool().data.b;
    default:
      break;
  }
  spdlog::error("VarValue::operator< - Wrong type");
  return false;
}

bool VarValue::operator<=(const VarValue& argument2) const
{
  ANTIGO_CONTEXT_INIT(ctx);
  ctx.AddMessage("next: type left, type right");
  ctx.AddUnsigned(GetType());
  ctx.AddUnsigned(argument2.GetType());

  switch (this->type) {
    case VarValue::kType_Integer:
      return this->CastToInt().data.i <= argument2.CastToInt().data.i;
    case VarValue::kType_Float:
      return this->CastToFloat().data.f <= argument2.CastToFloat().data.f;
    case VarValue::kType_Bool:
      return this->CastToBool().data.b <= argument2.CastToBool().data.b;
    default:
      break;
  }
  spdlog::error("VarValue::operator<= - Wrong type");
  return false;
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

std::string VarValue::ToString() const
{
  std::stringstream ss;
  ss << *this;
  return ss.str();
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
  if (arg2.type >= arg2._ArraysStart && arg2.type < arg2._ArraysEnd) {
    pArray = arg2.pArray;
  }

  promise = arg2.promise;

  if (arg2.stringHolder) {
    stringHolder.reset(new std::string(*arg2.stringHolder));
    data.string = stringHolder->data();
  } else {
    stringHolder.reset();
    // data.string ptr is copied by 'data = arg2.data;' line in this case
  }

  return *this;
}

VarValue VarValue::CastToString(const VarValue& var)
{
  switch (var.GetType()) {
    case VarValue::kType_Object: {
      IGameObject* ptr = ((IGameObject*)var);
      if (ptr) {
        return VarValue(ptr->GetStringID());
      } else {
        const static std::string noneString = "None";
        return VarValue(noneString.c_str());
      }
    }
    case VarValue::kType_Identifier:
      spdlog::error(
        "VarValue::CastToString - kType_Identifier can't be casted");
      return VarValue(std::string{});
    case VarValue::kType_String:
      return var;
    case VarValue::kType_Integer:
      return VarValue(std::to_string(static_cast<int32_t>(var)));
    case VarValue::kType_Float: {
      char buffer[512];
      snprintf(buffer, sizeof(buffer), "%.*g", 9000, static_cast<double>(var));
      return VarValue(std::string(buffer));
    }
    case VarValue::kType_Bool: {
      return VarValue(static_cast<bool>(var) ? "True" : "False");
    }
    case VarValue::kType_ObjectArray:
      return GetElementsArrayAtString(var, var.kType_ObjectArray);
    case VarValue::kType_StringArray:
      return GetElementsArrayAtString(var, var.kType_StringArray);
    case VarValue::kType_IntArray:
      return GetElementsArrayAtString(var, var.kType_IntArray);
    case VarValue::kType_FloatArray:
      return GetElementsArrayAtString(var, var.kType_FloatArray);
    case VarValue::kType_BoolArray:
      return GetElementsArrayAtString(var, var.kType_BoolArray);
    default:
      spdlog::error("VarValue::CastToString - Wrong type");
      return VarValue(std::string{});
  }
}

VarValue VarValue::GetElementsArrayAtString(const VarValue& array,
                                            uint8_t type)
{
  std::string returnValue = "[";

  for (size_t i = 0; i < array.pArray->size(); ++i) {
    switch (type) {
      case VarValue::kType_ObjectArray: {
        auto object = (static_cast<IGameObject*>((*array.pArray)[i]));
        returnValue += object ? object->GetStringID() : "None";
        break;
      }

      case VarValue::kType_StringArray:
        returnValue += (const char*)((*array.pArray)[i]);
        break;

      case VarValue::kType_IntArray:
        returnValue += std::to_string((int)((*array.pArray)[i]));
        break;

      case VarValue::kType_FloatArray:
        returnValue += std::to_string((double)((*array.pArray)[i]));
        break;

      case VarValue::kType_BoolArray: {
        VarValue& temp = ((*array.pArray)[i]);
        returnValue += (const char*)(CastToString(temp));
        break;
      }
      default:
        spdlog::error(
          "GetElementsArrayAtString - Unable to stringify element of type {}",
          static_cast<int>(type));
        break;
    }

    if (i < array.pArray->size() - 1) {
      returnValue += ", ";
    } else {
      returnValue += "]";
    }
  }

  return VarValue(returnValue);
}
