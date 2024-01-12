#pragma once
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <string>

class Quest
{
public:
  nlohmann::json ToJson() const;

  // Doesn't parse extra data currently
  static Quest FromJson(simdjson::dom::element& element);
  static Quest FromJson(const nlohmann::json& j);

  bool SetCurrentStageID(uint32_t stageID) const;
  void SetActive(bool active);
  uint32_t GetStage() const;
  bool GetStageDone(uint32_t stageID) const;
  void CompleteQuest();
  void CompleteAllObjectives();
  bool IsObjectiveCompleted(uint32_t objective) const;
  void SetObjectiveCompleted(uint32_t objective, bool completed);
  std::string GetID() const;
  uint32_t GetPriority() const;

  static Quest GetQuest(std::string editorID);

  // Fields

  // Operators
};
