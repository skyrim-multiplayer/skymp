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

double FridaHooksUtils::GetMenuNumberVariable(void* fsName, const char* target)
{
  if (auto mm = MenuManager::GetSingleton()) {
    if (auto view =
          mm->GetMovieView(reinterpret_cast<BSFixedString*>(fsName))) {
      GFxValue fxValue;
      view->GetVariable(&fxValue, target);
      return fxValue.GetNumber();
    }
  }
  return NULL;
}

void FridaHooksUtils::SaveCursorPosition()
{
  static auto fsCursorMenu = new BSFixedString("Cursor Menu");
  if (auto mm = MenuManager::GetSingleton()) {
    if (auto view =
          mm->GetMovieView(reinterpret_cast<BSFixedString*>(fsCursorMenu))) {
      GViewport vp;
      view->GetViewport(&vp);
      auto gr = view->GetVisibleFrameRect();
      GFxValue fxValueX;
      view->GetVariable(&fxValueX, "_root.mc_Cursor._x");
      GFxValue fxValueY;
      view->GetVariable(&fxValueY, "_root.mc_Cursor._y");
      auto cursorX = round((vp.width * (fxValueX.GetNumber() + abs(gr.left)) /
                            (abs(gr.right) + abs(gr.left))) *
                           10.0) /
        10.0;
      *GetCursorX() = cursorX;
      auto cursorY = round((vp.height * (fxValueY.GetNumber() + abs(gr.top)) /
                            (abs(gr.bottom) + abs(gr.top))) *
                           10.0) /
        10.0;
      *GetCursorY() = cursorY;
    }
  }
}

float* FridaHooksUtils::GetCursorX()
{
  static float g_cursorX = 0;
  return &g_cursorX;
}

float* FridaHooksUtils::GetCursorY()
{
  static float g_cursorY = 0;
  return &g_cursorY;
}
