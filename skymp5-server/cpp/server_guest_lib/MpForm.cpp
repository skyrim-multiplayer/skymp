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
  GetParent()->SendPapyrusEvent(this, eventName, arguments, argumentsCount);
}

VarValue MpForm::ToVarValue() const
{
  return VarValue(ToGameObject().get());
}

std::shared_ptr<IGameObject> MpForm::ToGameObject() const
{
  if (!gameObject)
    gameObject.reset(new MpFormGameObject(const_cast<MpForm*>(this)));
  return gameObject;
}
