#include "ReadBookEvent.h"

#include "MpActor.h"
#include "WorldState.h"
#include <string>
#include <unordered_set>
#include <vector>

ReadBookEvent::ReadBookEvent(MpActor* actor_, uint32_t baseId_)
  : actor(actor_)
  , baseId(baseId_)
{
}

const char* ReadBookEvent::GetName() const
{
  return "onReadBook";
}

std::string ReadBookEvent::GetArgumentsJsonArray() const
{
  std::string result;
  result += "[";
  result += std::to_string(actor->GetFormId());
  result += ",";
  result += std::to_string(baseId);
  result += "]";
  return result;
}

bool ReadBookEvent::SpellLearned() const
{
  return spellLearned;
}

void ReadBookEvent::OnFireSuccess(WorldState* worldState)
{
  auto& loader = actor->GetParent()->GetEspm();
  auto bookLookupResult = loader.GetBrowser().LookupById(baseId);

  if (!bookLookupResult.rec) {
    spdlog::error("ReadBookEvent::OnFireSuccess {:x} - No book form {:x}",
                  actor->GetFormId(), baseId);
    return;
  }

  const auto bookData = espm::GetData<espm::BOOK>(baseId, GetParent());
  const auto spellOrSkillFormId =
    bookLookupResult.ToGlobalId(bookData.spellOrSkillFormId);

  if (bookData.IsFlagSet(espm::BOOK::Flags::TeachesSpell)) {
    if (actor->ChangeForm().learnedSpells.IsSpellLearned(spellOrSkillFormId)) {
      spdlog::info("ReadBookEvent::OnFireSuccess {:x} - Spell already learned "
                   "{:x}, not spending the book",
                   GetFormId(), spellOrSkillFormId);
      return;
    }

    EditChangeForm([&](MpChangeForm& changeForm) {
      changeForm.learnedSpells.LearnSpell(spellOrSkillFormId);
    });
    spellLearned = true;
    return;
  } else if (bookData.IsFlagSet(espm::BOOK::Flags::TeachesSkill)) {
    spdlog::info("ReadBookEvent::OnFireSuccess {:x} - Skill book {:x} "
                 "detected, not implemented",
                 GetFormId(), baseId);
    return;
  }
}
