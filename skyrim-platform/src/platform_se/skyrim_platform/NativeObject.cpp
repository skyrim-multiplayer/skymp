#include "NativeObject.h"
#include "PapyrusTESModPlatform.h"

NativeObject::NativeObject(const CallNative::ObjectPtr& obj_)
  : papyrusUpdateId(TESModPlatform::GetNumPapyrusUpdates())
  , obj(obj_)
{
}

const CallNative::ObjectPtr& NativeObject::Get() const
{
  auto n = TESModPlatform::GetNumPapyrusUpdates();
  if (n != papyrusUpdateId) {
    std::stringstream ss;
    ss << "This game object is expired, consider saving form ID instead of "
          "the object itself (papyrusUpdateId="
       << papyrusUpdateId << ", n=" << n << ")";
    throw std::runtime_error(ss.str());
  }
  return obj;
}
