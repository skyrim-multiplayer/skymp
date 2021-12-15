#include "FridaHooksUtils.h"
#include <re/IMenu.h>
#include <skse64/GameMenus.h>

void* FridaHooksUtils::GetMenuByName(void* name)
{
  if (auto mm = MenuManager::GetSingleton()) {
    // auto* set = (tHashSet<MenuTableItem, BSFixedString>*)((uint8_t*)mm +
    // 128); return (void*)set->Size();
    return mm->GetMenu((BSFixedString*)name);
  }
  return nullptr;
}

bool FridaHooksUtils::SetMenuNumberVariable(void* fsName, const char* target,
                                            double value)
{
  if (auto mm = MenuManager::GetSingleton()) {
    auto view = mm->GetMovieView(reinterpret_cast<BSFixedString*>(fsName));
    if (view) {
      GFxValue fxValue;
      fxValue.SetNumber(value);
      view->SetVariable(target, &fxValue, 1);
      return true;
    }
  }
  return false;
}
