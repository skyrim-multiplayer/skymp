#include "NiPoint3.h"
#include <functional>
#include <map>
#include <memory>
#include <set>

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

namespace sweetpie {

struct Data
{
  enum InvalidId : uint64_t
  {
    InvalidId = (uint64_t)~0
  };
  uint64_t id = InvalidId;
  int64_t score = 0;
  NiPoint3 position;
  std::set<std::shared_ptr<sweetpie::Data>> subordinates;
};
}
