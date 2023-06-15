#pragma once
#include "WorldState.h"
#include <entt/entt.hpp>

class Entity
{
private:
  using underlying_t = entt::entity;
  using underlying_null_t = entt::null_t;

public:
  Entity(underlying_t entity_, const WorldState* context_);
  Entity(const Entity& entity_);
  Entity operator=(const Entity& entity_);
  Entity(Entity&& entity) = default;
  Entity& operator=(Entity&& entity) = default;

public:
  static Entity Null() noexcept;
  static uint32_t ToInt(Entity entity) noexcept;

public:
  template <typename... Component>
  decltype(auto) Get()
  {
    auto components = context->try_get<Component...>(id);
    auto valid = [this](auto&& component) {
      if (!static_cast<bool>(component)) {
        throw std::runtime_error(fmt::format(
          "Couldn't obtain the component of the entity associeated "
          "with entity id {:x}",
          Entity::ToInt(id));
      }
    };

    if constexpr (sizeof...(Component) == 1) {
      std::invoke(valid, components);
    } else {
      std::apply([valid](auto&&... component) { (..., valid(component)); },
                 components);
    }
    return context->get<Component...>(id);
  }

  template <typename... Component>
  decltype(auto) TryGet() noexcept
  {
    return context->try_get<Component...>(id);
  }

  template <typename Component, typename... Args>
  decltype(auto) Add(Args&&... args) noexcept
  {
    if (!context->valid(id)) {
      return;
    }
    return context->emplace<Component>(std::forward(args)...);
  }

  template <typename... Component>
  bool Has() noexcept
  {
    if (!context->valid(id)) {
      return;
    }
    return context->all_of<Component...>(entity);
  }

  template <typename... Component>
  bool HasAny() noexcept
  {
    if (!context->valid(id)) {
      return;
    }
    return context->any_of<Component...>(entity);
  }

  template <typename... Component>
  void Remove() noexcept
  {
    if (!context->valid(id)) {
      return;
    }
    context->remove<Component...>(entity);
  }

private:
  Entity();

private:
  underlying_t id;
  const WorldState* context;

  friend class WorldState;
};
