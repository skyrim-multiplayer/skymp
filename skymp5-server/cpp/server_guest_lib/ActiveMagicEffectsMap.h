#pragma once
#include "libespm/espm.h"
#include <chrono>
#include <nlohmann/json.hpp>
#include <optional>
#include <simdjson.h>
#include <unordered_map>

class ActiveMagicEffectsMap
{
public:
  struct Entry
  {
    espm::Effects::Effect data;
    std::chrono::system_clock::time_point endTime;
  };

public:
  static ActiveMagicEffectsMap FromJson(const simdjson::dom::element& effects);

public:
  template <typename T>
  void Add(espm::ActorValue actorValue, T&& entry)
  {
    auto it = effects.find(actorValue);
    effects[actorValue] = std::forward<T>(entry);
  }

  std::vector<espm::Effects::Effect> GetAllEffects() const noexcept;

  std::optional<std::reference_wrapper<const Entry>> Get(
    espm::ActorValue actorValue) const noexcept;
  void Remove(espm::ActorValue actorValue) noexcept;
  void Clear() noexcept;
  bool Has(espm::ActorValue actorValue) const noexcept;
  [[nodiscard]] bool Empty() const noexcept;
  nlohmann::json::array_t ToJson() const;

private:
  std::unordered_map<espm::ActorValue, Entry> effects;
};
