#pragma once
#include "IPapyrusClass.h"

class PapyrusGame final : public IPapyrusClass<PapyrusGame>
{
public:
  const char* GetName() override { return "Game"; }

  VarValue ForceThirdPerson(VarValue self,
                            const std ::vector<VarValue>& arguments);
  VarValue DisablePlayerControls(VarValue self,
                                 const std ::vector<VarValue>& arguments);
  VarValue EnablePlayerControls(VarValue self,
                                const std ::vector<VarValue>& arguments);
  VarValue IncrementStat(VarValue self,
                         const std::vector<VarValue>& arguments);

  // In Skyrim it's not a native, just a wrapper around
  // FindClosestReferenceOfAnyTypeInList
  // TODO: implement FindClosestReferenceOfAnyTypeInList native instead
  VarValue FindClosestReferenceOfAnyTypeInListFromRef(
    VarValue self, const std::vector<VarValue>& arguments);

  // In Skyrim it's not a native, just a wrapper around
  // FindClosestReferenceOfType
  // TODO: implement FindClosestReferenceOfType native instead
  VarValue FindClosestReferenceOfTypeFromRef(
    VarValue self, const std::vector<VarValue>& arguments);

  VarValue GetPlayer(VarValue self, const std::vector<VarValue>& arguments);
  VarValue ShowRaceMenu(VarValue self, const std::vector<VarValue>& arguments);
  VarValue ShowLimitedRaceMenu(VarValue self,
                               const std::vector<VarValue>& arguments);
  VarValue GetCameraState(VarValue self,
                          const std::vector<VarValue>& arguments);
  VarValue GetForm(VarValue self,
                   const std::vector<VarValue>& arguments) const noexcept;
  VarValue GetFormEx(VarValue self,
                     const std::vector<VarValue>& arguments) const noexcept;
  VarValue ShakeController(VarValue self,
                           const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

private:
  void RaceMenuHelper(VarValue& self, const char* funcName,
                      const std::vector<VarValue>& arguments);
  VarValue GetFormInternal(VarValue self,
                           const std::vector<VarValue>& arguments,
                           bool extended) const noexcept;
};
