#include "PapyrusClassesFactory.h"

#include "PapyrusActor.h"
#include "PapyrusCell.h"
#include "PapyrusDebug.h"
#include "PapyrusEffects.h"
#include "PapyrusFaction.h"
#include "PapyrusForm.h"
#include "PapyrusFormList.h"
#include "PapyrusGame.h"
#include "PapyrusKeyword.h"
#include "PapyrusLeveledObjects.h"
#include "PapyrusMessage.h"
#include "PapyrusNetImmerse.h"
#include "PapyrusObjectReference.h"
#include "PapyrusPotion.h"
#include "PapyrusQuest.h"
#include "PapyrusSkymp.h"
#include "PapyrusSound.h"
#include "PapyrusUtility.h"

std::vector<std::unique_ptr<IPapyrusClassBase>>
PapyrusClassesFactory::CreateAndRegister(
  VirtualMachine& vm,
  const std::shared_ptr<IPapyrusCompatibilityPolicy>& compatibilityPolicy)
{
  std::vector<std::unique_ptr<IPapyrusClassBase>> result;

  result.emplace_back(std::make_unique<PapyrusObjectReference>());
  result.emplace_back(std::make_unique<PapyrusGame>());
  result.emplace_back(std::make_unique<PapyrusForm>());
  result.emplace_back(std::make_unique<PapyrusMessage>());
  result.emplace_back(std::make_unique<PapyrusFormList>());
  result.emplace_back(std::make_unique<PapyrusDebug>());
  result.emplace_back(std::make_unique<PapyrusActor>());
  result.emplace_back(std::make_unique<PapyrusSkymp>());
  result.emplace_back(std::make_unique<PapyrusUtility>());
  result.emplace_back(std::make_unique<PapyrusEffects>("EffectShader"));
  result.emplace_back(std::make_unique<PapyrusKeyword>());
  result.emplace_back(std::make_unique<PapyrusFaction>());
  result.emplace_back(std::make_unique<PapyrusCell>());
  result.emplace_back(std::make_unique<PapyrusSound>());
  result.emplace_back(std::make_unique<PapyrusNetImmerse>());
  result.emplace_back(std::make_unique<PapyrusPotion>());
  result.emplace_back(std::make_unique<PapyrusEffects>("VisualEffect"));
  result.emplace_back(std::make_unique<PapyrusQuest>());
  result.emplace_back(std::make_unique<PapyrusLeveledObjects>("LeveledItem"));
  result.emplace_back(std::make_unique<PapyrusLeveledObjects>("LeveledSpell"));

  for (auto& papyrusClass : result) {
    papyrusClass->Register(vm, compatibilityPolicy);
  }

  return result;
}
