#include "ReadBookEvent.h"

#include "MpActor.h"
#include "WorldState.h"
#include "script_objects/EspmGameObject.h"
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
    spdlog::error("ReadBookEvent::OnFireSuccess - Actor {:x} reading book: No "
                  "book form {:x}",
                  actor->GetFormId(), baseId);
    return;
  }

  const auto bookData = espm::GetData<espm::BOOK>(baseId, actor->GetParent());
  const auto spellOrSkillFormId =
    bookLookupResult.ToGlobalId(bookData.spellOrSkillFormId);

  if (bookData.IsFlagSet(espm::BOOK::Flags::TeachesSpell)) {
    if (actor->IsSpellLearned(spellOrSkillFormId)) {
      spdlog::info("ReadBookEvent::OnFireSuccess - Actor {:x} reading book: "
                   "Spell already learned "
                   "{:x}, not spending the book",
                   actor->GetFormId(), spellOrSkillFormId);
      return;
    }
    actor->AddSpell(spellOrSkillFormId);
    spellLearned = true;
    return;
  } else if (bookData.IsFlagSet(espm::BOOK::Flags::TeachesSkill)) {
    spdlog::info(
      "ReadBookEvent::OnFireSuccess - Actor {:x} reading skill book {:x} "
      "detected, not implemented",
      actor->GetFormId(), baseId);
    return;
  }
}

void ReadBookEvent::OnFireBlocked(WorldState* worldState) override
{
  auto& loader = actor->GetParent()->GetEspm();
  auto bookLookupResult = loader.GetBrowser().LookupById(baseId);

  const auto bookData = espm::GetData<espm::BOOK>(baseId, actor->GetParent());
  const auto spellOrSkillFormId =
    bookLookupResult.ToGlobalId(bookData.spellOrSkillFormId);

  if (!bookData.IsFlagSet(espm::BOOK::Flags::TeachesSpell)) {
    return;
  }

  auto aSpell = VarValue(std::make_shared<EspmGameObject>(
    loader.GetBrowser().LookupById(spellOrSkillFormId)));

  std::vector<VarValue> arguments = { aSpell };

  SpSnippet("Actor", "RemoveSpell",
            SpSnippetFunctionGen::SerializeArguments(arguments).data(),
            actor->GetFormId())
    .Execute(actor, SpSnippetMode::kNoReturnResult);
}
