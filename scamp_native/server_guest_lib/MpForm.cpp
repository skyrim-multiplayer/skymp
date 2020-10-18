#include "MpForm.h"

#include "MpFormGameObject.h"
#include "WorldState.h"

MpForm::MpForm()
{
}

void MpForm::Init(WorldState* parent_, uint32_t formId_)
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
  GetParent()->GetPapyrusVm().SendEvent(
    ToGameObject(), eventName, { arguments, arguments + argumentsCount });
}

VarValue MpForm::ToVarValue() const
{
  if (!gameObject)
    gameObject.reset(new MpFormGameObject(const_cast<MpForm*>(this)));
  return VarValue(gameObject.get());
}

std::shared_ptr<IGameObject> MpForm::ToGameObject() const
{
  return gameObject;
}