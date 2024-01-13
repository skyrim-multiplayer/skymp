#pragma once
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <string>

struct Quest
{
  // Move as Papyrus Commands

  // void Start();
  // void End();
  // bool SetStage(uint32_t stageID) const;
  // void SetActive(bool active);
  // uint32_t GetStage() const;
  // bool GetStageDone(uint32_t stageID) const;
  // void CompleteQuest();
  // void CompleteAllObjectives();
  // bool IsObjectiveCompleted(uint32_t objective) const;
  // void SetObjectiveCompleted(uint32_t objective, bool completed);
  // bool IsObjectiveDisplayed(uint32_t objective) const;
  // void SetObjectiveDisplayed(uint32_t objective, bool displayed);  
  // bool IsObjectiveFailed(uint32_t objective) const;
  // void SetObjectiveFailed(uint32_t objective, bool failed);
  // std::string GetID() const;
  // uint32_t GetPriority() const;
  // static Quest GetQuest(std::string editorID);

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

  friend bool operator!=(const Quest& r, const Quest& l)
  {
    return !(r == l);
  }
};
