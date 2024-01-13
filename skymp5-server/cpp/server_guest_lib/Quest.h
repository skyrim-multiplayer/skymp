#pragma once
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <string>

class Quest
{
public:
  bool SetStage(uint32_t stageID) const;
  void SetActive(bool active);
  uint32_t GetStage() const;
  bool GetStageDone(uint32_t stageID) const;
  void CompleteQuest();
  void CompleteAllObjectives();
  bool IsObjectiveCompleted(uint32_t objective) const;
  void SetObjectiveCompleted(uint32_t objective, bool completed);
  bool IsObjectiveDisplayed(uint32_t objective) const;
  void SetObjectiveDisplayed(uint32_t objective, bool displayed);
  bool IsObjectiveFailed(uint32_t objective) const;
  void SetObjectiveFailed(uint32_t objective, bool failed);
  std::string GetID() const;
  uint32_t GetPriority() const;

  static Quest GetQuest(std::string editorID);

  // public due we need to assign this values initially
  std::string editorID;
  uint32_t priority;

private:
  std::vector<uint32_t> unlockedStages;
  std::vector<uint32_t> unlockedObjectives;

  uint32_t displayedObjective;
  bool completed;
};
