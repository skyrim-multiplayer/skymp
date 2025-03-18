#pragma once
#include "IPapyrusClass.h"

class PapyrusUtility final : public IPapyrusClass<PapyrusUtility>
{
public:
  const char* GetName() override { return "Utility"; }

  VarValue Wait(VarValue self, const std::vector<VarValue>& arguments);
  VarValue RandomInt(VarValue self, const std::vector<VarValue>& arguments);
  VarValue RandomFloat(VarValue self, const std::vector<VarValue>& arguments);
  VarValue GetCurrentRealTime(VarValue self,
                              const std::vector<VarValue>& arguments);
  VarValue WaitMenuMode(VarValue self, const std::vector<VarValue>& arguments);
  VarValue WaitGameTime(VarValue self, const std::vector<VarValue>& arguments);
  VarValue IsInMenuMode(VarValue self, const std::vector<VarValue>& arguments);
  VarValue GetCurrentGameTime(VarValue self,
                              const std::vector<VarValue>& arguments);
  VarValue GameTimeToString(VarValue self,
                            const std::vector<VarValue>& arguments);
  VarValue CreateAliasArray(VarValue self,
                            const std::vector<VarValue>& arguments);
  VarValue CreateBoolArray(VarValue self,
                           const std::vector<VarValue>& arguments);
  VarValue CreateFloatArray(VarValue self,
                            const std::vector<VarValue>& arguments);
  VarValue CreateFormArray(VarValue self,
                           const std::vector<VarValue>& arguments);
  VarValue CreateIntArray(VarValue self,
                          const std::vector<VarValue>& arguments);
  VarValue ResizeAliasArray(VarValue self,
                            const std::vector<VarValue>& arguments);
  VarValue ResizeBoolArray(VarValue self,
                           const std::vector<VarValue>& arguments);
  VarValue ResizeFloatArray(VarValue self,
                            const std::vector<VarValue>& arguments);
  VarValue ResizeFormArray(VarValue self,
                           const std::vector<VarValue>& arguments);
  VarValue ResizeIntArray(VarValue self,
                          const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

private:
  VarValue WaitHelper(VarValue& self, const char* funcName,
                      const std::vector<VarValue>& arguments);
  VarValue ArrayHelper(VarValue& self, const char* funcName,
                       const std::vector<VarValue>& arguments,
                       VarValue::Type type, bool resize = false);
};
