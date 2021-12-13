#include "FridaHooksUtils.h"

void* FridaHooksUtils::GetMenuByName(std::string_view name)
{
  if (auto ui = RE::UI::GetSingleton()) {
    // auto* set = (tHashSet<MenuTableItem, BSFixedString>*)((uint8_t*)mm +
    // 128); return (void*)set->Size();
    return ui->GetMenu(name);
  }
  return nullptr;
}
