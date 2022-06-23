#pragma once

namespace FridaHooksUtils {

void* GetMenuByName(std::string_view name)
{
  if (auto ui = RE::UI::GetSingleton())
    return (void*)ui->GetMenu(name).get();
  return nullptr;
}

double GetMenuNumberVariable(std::string_view menuName, const char* target)
{
  auto ui = RE::UI::GetSingleton();
  if (!ui)
    return NULL;

  auto view = ui->GetMovieView(menuName);
  if (!view)
    return NULL;

  RE::GFxValue fxValue;
  view->GetVariable(&fxValue, target);

  return fxValue.GetNumber();
}

bool SetMenuNumberVariable(std::string_view menuName, const char* target,
                           double value)
{
  auto ui = RE::UI::GetSingleton();
  if (!ui)
    return false;

  auto view = ui->GetMovieView(menuName);
  if (!view)
    return false;

  RE::GFxValue fxValue;
  fxValue.SetNumber(value);

  return view->SetVariable(target, fxValue, RE::GFxMovie::SetVarType::kSticky);
}

}
