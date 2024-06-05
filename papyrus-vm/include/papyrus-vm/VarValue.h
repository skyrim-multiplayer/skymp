#pragma once
#include "Promise.h"
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

class StackIdHolder;
class IGameObject;

struct VarValue
{
private:
  union
  {
    IGameObject* id;
    const char* string;
    int32_t i;
    double f;
    bool b;
  } data;

  std::shared_ptr<IGameObject> owningObject;
  int32_t stackId = -1;

public:
  std::string objectType;

  enum Type : uint8_t
  {
    kType_Object = 0, // 0 null?
    kType_Identifier, // 1 identifier
    kType_String,     // 2
    kType_Integer,    // 3
    kType_Float,      // 4
    kType_Bool,       // 5

    _ArraysStart = 11,
    kType_ObjectArray = 11,
    kType_StringArray = 12,
    kType_IntArray = 13,
    kType_FloatArray = 14,
    kType_BoolArray = 15,
    _ArraysEnd = 16,
  };

  uint8_t GetType() const { return static_cast<uint8_t>(this->type); }

  VarValue()
  {
    data.id = nullptr;
    type = Type::kType_Object;
  }

  explicit VarValue(uint8_t type);
  explicit VarValue(IGameObject* object);
  explicit VarValue(int32_t value);
  explicit VarValue(const char* value);
  explicit VarValue(const std::string& value);
  explicit VarValue(double value);
  explicit VarValue(bool value);
  explicit VarValue(Viet::Promise<VarValue> promise);
  explicit VarValue(std::shared_ptr<IGameObject> object);

  VarValue(uint8_t type, const char* value);

  static VarValue None() { return VarValue(); }

  explicit operator bool() const { return CastToBool().data.b; }

  explicit operator IGameObject*() const { return data.id; }

  explicit operator int() const { return CastToInt().data.i; }

  explicit operator double() const { return CastToFloat().data.f; }

  explicit operator const char*() const { return data.string; }

  std::shared_ptr<std::vector<VarValue>> pArray;

  std::shared_ptr<Viet::Promise<VarValue>> promise;

  std::shared_ptr<std::string> stringHolder;

  int32_t GetMetaStackId() const;
  void SetMetaStackIdHolder(std::shared_ptr<StackIdHolder> stackIdHolder);
  static VarValue AttachTestStackId(VarValue original = VarValue::None(),
                                    int32_t stackId = 108);

  VarValue operator+(const VarValue& argument2);
  VarValue operator-(const VarValue& argument2);
  VarValue operator*(const VarValue& argument2);
  VarValue operator/(const VarValue& argument2);
  VarValue operator%(const VarValue& argument2);
  VarValue operator!();

  bool operator==(const VarValue& argument2) const;
  bool operator!=(const VarValue& argument2) const;
  bool operator>(const VarValue& argument2) const;
  bool operator>=(const VarValue& argument2) const;
  bool operator<(const VarValue& argument2) const;
  bool operator<=(const VarValue& argument2) const;

  friend std::ostream& operator<<(std::ostream& os, const VarValue& varValue);
  std::string ToString() const;

  VarValue& operator=(const VarValue& arg2);

  VarValue CastToInt() const;
  VarValue CastToFloat() const;
  VarValue CastToBool() const;

  void Then(std::function<void(VarValue)> cb);

  static VarValue CastToString(const VarValue& var);

  static VarValue GetElementsArrayAtString(const VarValue& array,
                                           uint8_t type);

private:
  Type type;
};
