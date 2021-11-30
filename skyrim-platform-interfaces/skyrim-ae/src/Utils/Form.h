namespace Papyrus::Utils {

/* this relies on correctly provided T and type for now */
template <class T>
[[nodiscard]] T* CreateForm(RE::FormType type)
{
  auto factory = RE::IFormFactory::GetFormFactoryByType(type);
  if (!factory)
    return nullptr;

  return (T)factory->Create();
}

}
