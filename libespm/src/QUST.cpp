#include "libespm/QUST.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>
#include <spdlog/spdlog.h>

namespace espm {

std::vector<QUST::QuestObjective> QUST::GetQuestObjectives(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  std::vector<QuestObjective> result;

  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "QOBJ", 4)) {
        result.push_back(QuestObjective());
        result.back().index = *reinterpret_cast<const int16_t*>(data);
      } else if (!std::memcmp(type, "FNAM", 4) && result.size() != 0) {
        result.back().flags = *reinterpret_cast<const int32_t*>(data);
      } else if (!std::memcmp(type, "NNAM", 4) && result.size() != 0) {
        result.back().nodeName = data;
      } else if (!std::memcmp(type, "QSTA", 4) && result.size() != 0) {
        result.back().questTargets.push_back(
          *reinterpret_cast<const QuestObjective::QuestTarget*>(data));
      }
    },
    compressedFieldsCache);
  return result;
}

std::vector<QUST::QuestStage> QUST::GetQuestStages(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  std::vector<QuestStage> result;

  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "INDX", 4)) {
        result.push_back(QuestStage());
        result.back().journalIndex =
          reinterpret_cast<const QuestStage::JournalIndex*>(data);
      } else if (!std::memcmp(type, "QSDT", 4)) {
        result.back().logEntries.push_back(QuestStage::QuestLogEntry());
        result.back().logEntries.back().logFlags =
          *reinterpret_cast<const QuestStage::QuestLogEntry::LogFlags*>(data);
      } else if (!std::memcmp(type, "CNAM", 4)) {
        result.back().logEntries.back().journalEntry = data;
      }
    },
    compressedFieldsCache);
  return result;
}

QUST::Data QUST::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;

  int questStageID = -1;
  int questLogEntryID = -1;

  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "EDID", 4)) {
        result.editorId = data;
      } else if (!std::memcmp(type, "VMAD", 4)) {
        GetScriptData(&result.scriptData, compressedFieldsCache);
      } else if (!std::memcmp(type, "FULL", 4)) {
        result.questName = data;
      } else if (!std::memcmp(type, "DNAM", 4)) {
        result.questData = reinterpret_cast<const QuestData*>(data);
      } else if (!std::memcmp(type, "ENAM", 4)) {
        result.eventName = data;
      } else if (!std::memcmp(type, "QTGL", 4)) {
        result.displayGlobals = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "FLTR", 4)) {
        result.objectWindowFilter = data;
      } else if (!std::memcmp(type, "QuestObjectives", 4)) {
        result.nextAliasId = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "ANAM", 4)) {
        result.nextAliasId = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "Aliases", 4)) {
        result.nextAliasId = *reinterpret_cast<const uint32_t*>(data);
      }
    },
    compressedFieldsCache);

  result.questStages = GetQuestStages(compressedFieldsCache);
  result.questObjectives = GetQuestObjectives(compressedFieldsCache);

  return result;
}

}
