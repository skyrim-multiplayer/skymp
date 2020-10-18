#include "MpForm.h"

#include "MpFormGameObject.h"

MpForm::MpForm()
{
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