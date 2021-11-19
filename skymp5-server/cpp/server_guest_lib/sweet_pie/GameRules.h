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
  std::set<std::shared_ptr<sweetpie::Data>> subordinates;
  sweetpie::geometry::SimpleFugure space;
};
}

namespace sweetpie {
namespace geometry {
enum class Figure
{
  Null,
  Circle,
  Ellipse,
  Rectangle
};
class SimpleFugure
{
public:
  SimpleFugure() = delete;
  virtual bool IsInside(NiPoint3 pos) { return true; }
  const NiPoint3& GetPosition() { return position; }

protected:
  NiPoint3 position;
  float sqrNorm = 0;
  float norm = 0;
  Figure type = Figure::Null;
};

class Circle : public sweetpie::geometry::SimpleFugure
{
public:
  Circle(NiPoint3 position, float radius);
  const float& GetRadius() const;
  bool IsInside(NiPoint3 pos) override;

private:
  float radius = 0;
};

class Ellipse : public sweetpie::geometry::Circle
{
  friend sweetpie::geometry::Circle;

public:
  Ellipse(NiPoint3 position1, float radius1, NiPoint3 position2,
          float radius2);
  const float& GetSecondRadius() const;
  const NiPoint3& GetSecondPosition() const;
  bool IsInside(NiPoint3 pos) override;

private:
  float secondRadius = 0;
  NiPoint3 secondPosition;
};
class Rectangle : public sweetpie::geometry::SimpleFugure
{
public:
  Rectangle(NiPoint3 position, NiPoint3 size, float roatation);

private:
  NiPoint3 size;
  float roatation;
};
class Polyhedron : public sweetpie::geometry::SimpleFugure
{
private:
};

// represents Line x*A + y*B + C = 0
class Line
{
public:
  float A = 0.f;
  float B = 1.f;
  float C = 0.f;

private:
  float normalization = 1.f;
};

}
}
