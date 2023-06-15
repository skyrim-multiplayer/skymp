#include "Entity.h"

Entity::Entity(underlying_t entity_, const WorldState* context_)
  : id(entity_)
  , context(context_)
{
}

Entity::Entity()
  : id(underlying_null_t{})
  , context(nullptr)
{
}

Entity::Entity(const Entity& entity_)
  : id(entity_.id)
  , context(entity_.context)
{
}

Entity Entity::operator=(const Entity& entity_)
{
  id = entity_.id;
  context = entity_.context;
  return *this;
}

Entity Entity::Null() noexcept
{
  return Entity{};
}

uint32_t Entity::ToInt(Entity entity) noexcept
{
  return static_cast<uint32_t>(entity.id);
}
