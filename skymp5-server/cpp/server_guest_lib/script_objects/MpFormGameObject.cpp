#include "MpFormGameObject.h"

#include "WorldState.h"

#include <fmt/format.h>
#include <unordered_map>

MpFormGameObject::MpFormGameObject(MpForm* form_)
  : form(form_)
  , parent(form_ ? form_->GetParent() : nullptr)
  , formId(form_ ? form_->GetFormId() : 0)
{
}

MpForm* MpFormGameObject::GetFormPtr() const noexcept
{
  if (parent) {
    bool formStillValid = parent->LookupFormById(formId).get() == form;
    if (!formStillValid) {
      return nullptr;
    }
  }
  return form;
}

const char* MpFormGameObject::GetParentNativeScript()
{
  if (auto form = GetFormPtr())
    return form->GetFormType();
  return "";
}

bool MpFormGameObject::EqualsByValue(const IGameObject& obj) const
{
  if (auto form = dynamic_cast<const MpFormGameObject*>(&obj)) {
    return form->formId == formId;
  }
  return false;
}

const char* MpFormGameObject::GetStringID()
{
  static std::unordered_map<uint32_t, std::shared_ptr<std::string>> g_strings;
  auto formId = form->GetFormId();
  auto& v = g_strings[formId];
  if (!v) {
    v.reset(new std::string(fmt::format("form {:x}", formId)));
  }
  return v->data();
}

const std::vector<std::shared_ptr<ActivePexInstance>>&
MpFormGameObject::ListActivePexInstances() const
{
  static const std::vector<std::shared_ptr<ActivePexInstance>>
    kEmptyScriptsArray;

  auto form = GetFormPtr();
  if (!form) {
    return kEmptyScriptsArray;
  }
  return form->ListActivePexInstances();
}

void MpFormGameObject::AddScript(
  std::shared_ptr<ActivePexInstance> script) noexcept
{
  auto form = GetFormPtr();
  if (!form) {
    spdlog::critical("MpFormGameObject::AddScript - Invalid form");
    std::terminate();
  }
  return form->AddScript(script);
}
