#pragma once
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <string>

struct Quest
{
  // Move to PapyrusQuest

  // void StartQuest(std::string editorID);
  // void EndQuest(std::string editorID);
  // bool SetStage(std::string editorID, uint32_t stageID) const;
  // void SetActive(std::string editorID, bool active);
  // uint32_t GetStage(std::string editorID) const;
  // bool GetStageDone(std::string editorID, uint32_t stageID) const;
  // void CompleteQuest(std::string editorID);
  // void CompleteAllObjectives(std::string editorID);
  // bool IsObjectiveCompleted(std::string editorID, uint32_t objective) const;
  // void SetObjectiveCompleted(std::string editorID, uint32_t objective, bool completed);
  // bool IsObjectiveDisplayed(std::string editorID, uint32_t objective) const;
  // void SetObjectiveDisplayed(std::string editorID, uint32_t objective, bool displayed);
  // bool IsObjectiveFailed(std::string editorID, uint32_t objective) const;
  // void SetObjectiveFailed(std::string editorID, uint32_t objective, bool failed);
  // Quest GetQuest(std::string editorID); (Function)

  std::string editorID;
  uint32_t priority;

  std::vector<uint32_t> unlockedStages;
  std::vector<uint32_t> unlockedObjectives;
  std::vector<uint32_t> failedObjectives;

  uint32_t currentStageID;
  uint32_t displayedObjective;
  bool completed;

  friend bool operator==(const Quest& r, const Quest& l)
  {
    return r.editorID == l.editorID;
  }

  friend bool operator!=(const Quest& r, const Quest& l) { return !(r == l); }
};
