#include <map>
#include <memory>

namespace sweetpie {
class Rules
{
public:
  virtual void AddPlayer(uint64_t playerID);
  virtual void RemovePlayer(uint64_t playerID);
  virtual void BeforeUpdate();
  void Update();
  virtual void OnUpdate();
  virtual void CanMove();
  void OnMove();

private:
  struct RulesImpl;
  std::shared_ptr<RulesImpl> rulesImpl;
};
}

namespace sweetpie {
namespace gamemode {
class TeamPlay : public Rules
{
private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
}
}
