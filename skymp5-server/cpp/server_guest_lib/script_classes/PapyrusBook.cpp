#include "PapyrusBook.h"
#include "script_objects/EspmGameObject.h"

VarValue PapyrusBook::GetSpell(VarValue self,
                               const std::vector<VarValue>& arguments)
{
  auto selfRec = GetRecordPtr(self);
  auto bookData =
    espm::GetData<espm::BOOK>(selfRec.ToGlobalId(selfRec.rec->GetId()),
                              compatibilityPolicy->GetWorldState());

  auto spellOrSkillFormId = selfRec.ToGlobalId(bookData.spellOrSkillFormId);

  if (!spellOrSkillFormId) {
    spdlog::info("Book.GetSpell - Returning None, book contains no formId");
    return VarValue::None();
  }

  auto spellOrSkill =
    compatibilityPolicy->GetWorldState()->GetEspm().GetBrowser().LookupById(
      spellOrSkillFormId);

  if (!spellOrSkill.rec) {
    spdlog::warn("Book.GetSpell - Returning None, book contains a formId {:x} "
                 "which can't be found",
                 spellOrSkillFormId);
    return VarValue::None();
  }

  auto spellOrSkillType = spellOrSkill.rec->GetType();

  if (spellOrSkillType != espm::SPEL::kType) {
    spdlog::info(
      "Book.GetSpell - Returning None, book contains not a spell, but {}",
      spellOrSkillType.ToString());
    return VarValue::None();
  }

  return VarValue(std::make_shared<EspmGameObject>(spellOrSkill))
}

void PapyrusBook::Register(VirtualMachine& vm,
                           std::shared_ptr<IPapyrusCompatibilityPolicy> policy)
{
  compatibilityPolicy = policy;

  AddMethod(vm, "GetSpell", &PapyrusBook::GetSpell);
}
