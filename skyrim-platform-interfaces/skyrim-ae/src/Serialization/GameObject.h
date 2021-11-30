/**
 * We pass objects between interface and platform that comply to type
 * std::variant<std::shared_ptr<GameObject>, double, bool, std::string>;
 */

namespace Serialization {

class GameObject
{
public:
  GameObject(const char* a_type, void* a_obj)
    : type(a_type)
    , obj(a_obj)
  {
  }

  const char* GetType() const { return type; }
  void* GetObject() const { return obj; }

private:
  const char* const type;
  void* const obj;
};

}
