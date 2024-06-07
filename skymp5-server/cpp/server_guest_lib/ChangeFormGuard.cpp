#include "ChangeFormGuard.h"
#include "WorldState.h"

void ChangeFormGuard_::RequestSave(MpObjectReference* self)
{
  [[likely]] if (auto worldState = self->GetParent()) {
    worldState->RequestSave(*self);
  }
}
