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