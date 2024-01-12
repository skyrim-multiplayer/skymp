#pragma once

class Quest
{
public:
  nlohmann::json ToJson() const;

  // Doesn't parse extra data currently
  static Inventory FromJson(simdjson::dom::element& element);
  static Inventory FromJson(const nlohmann::json& j);

  // Fields

  // Operators
};
