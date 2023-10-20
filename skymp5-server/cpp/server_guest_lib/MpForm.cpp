#include "MpForm.h"

#include "WorldState.h"
#include "script_objects/MpFormGameObject.h"

MpForm::MpForm()
{
}

void MpForm::Init(WorldState* parent_, uint32_t formId_, bool hasChangeForm)
{
  parent = parent_;
  id = formId_;
};

void MpForm::Update()
{
  SendPapyrusEvent("OnUpdate");
}

void MpForm::SendPapyrusEvent(const char* eventName, const VarValue* arguments,
                              size_t argumentsCount)
{
  GetParent()->SendPapyrusEvent(this, eventName, arguments, argumentsCount);
}

VarValue MpForm::ToVarValue() const
{
  // TODO: consider using owning VarValue instead of non-owning
  return VarValue(ToGameObject().get());
}

std::shared_ptr<IGameObject> MpForm::ToGameObject() const
{
  if (!gameObject) {
    gameObject.reset(new MpFormGameObject(const_cast<MpForm*>(this)));
  }
  return gameObject;
}

const std::vector<std::shared_ptr<ActivePexInstance>>&
MpForm::ListActivePexInstances() const
{
  return activePexInstances;
}

void MpForm::AddScript(
  const std::shared_ptr<ActivePexInstance>& script) noexcept
{
  const std::string& name = script->GetSourcePexName();
  auto it = std::find_if(activePexInstances.begin(), activePexInstances.end(),
                         [&](const std::shared_ptr<ActivePexInstance>& item) {
                           return item->GetSourcePexName() == name;
                         });
  if (it != activePexInstances.end()) {
    spdlog::warn("MpForm::AddScript {:x} - Already added script name {}",
                 GetFormId(), name);
    return;
  }
  activePexInstances.push_back(script);
}
