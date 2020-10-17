#include "ChangeFormGuard.h"
#include "WorldState.h"

void ChangeFormGuard_::RequestSave(MpObjectReference* self)
{
  self->GetParent()->RequestSave(*self);
}