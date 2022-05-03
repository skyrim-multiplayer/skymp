#include "FLObjectReference.h"
#include "FunctionsLibApi.h"

void RegisterObjectReferenceApi(std::shared_ptr<PartOne> partOne)
{
  JsValue globalObject = JsValue::GlobalObject();

  // ObjectReference ctor
  globalObject.SetProperty(
    "ObjectReference",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return ObjectReferenceCtor(partOne, args);
    }));

  JsValue objectReference = globalObject.GetProperty("ObjectReference");
  objectReference.SetProperty(
    "prototype", globalObject.GetProperty("Form").GetProperty("prototype"));

  JsValue objectReferencePrototype = objectReference.GetProperty("prototype");

  objectReferencePrototype.SetProperty(
    "Activate", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return Activate(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "AddItem", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return AddItem(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "Disable", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return Disable(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "DoReloot", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return DoReloot(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "GetAngle", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetAngle(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "GetAnimationVariableBool",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetAnimationVariableBool(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "GetBaseId", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetBaseId(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "GetCellOrWorld",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetCellOrWorld(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "GetInventory",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetInventory(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "GetPos", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetPos(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "IsActivationBlocked",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return IsActivationBlocked(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "IsDisabled",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return IsDisabled(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "IsHarvested",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return IsHarvested(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "IsOpen", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return IsOpen(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "RemoveAllItems",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return RemoveAllItems(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "RemoveItem",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return RemoveItem(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "SetActivationBlocked",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return SetActivationBlocked(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "SetAngle", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return SetAngle(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "SetAnimationVariableBool",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return SetAnimationVariableBool(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "SetCellOrWorld",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return SetCellOrWorld(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "SetHarvested",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return SetHarvested(partOne, args);
    }));

  objectReferencePrototype.SetProperty(
    "SetPos", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return SetPos(partOne, args);
    }));
}

JsValue ObjectReferenceCtor(std::shared_ptr<PartOne> partOne,
                            const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[1]);
  if (formId == 0) {
    partOne->GetLogger().error("Error on check formId");
    return JsValue::Undefined();
  }

  args[0].SetProperty("_formId", args[1]);

  if (formId >= 0xff000000) {
    return JsValue::Undefined();
  }

  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);

  if (!lookupRes.rec || lookupRes.rec->GetType().ToString() != "REFR") {
    partOne->GetLogger().error("ObjectReference not exists");
    return JsValue::Undefined();
  }

  return JsValue::Undefined();
}

JsValue Activate(std::shared_ptr<PartOne> partOne,
                 const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto actorFormId = Uint32FromJsValue(args[0].GetProperty("_formId"));

  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  auto& actor = partOne->worldState.GetFormAt<MpActor>(actorFormId);
  obj.Activate(actor);

  return JsValue::Undefined();
}

JsValue AddItem(std::shared_ptr<PartOne> partOne,
                const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto itemFormId = Uint32FromJsValue(args[1]);
  auto count = Uint32FromJsValue(args[2]);

  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  if (count == 0) {
    count = 1;
  }

  obj.AddItem(itemFormId, count);

  return JsValue::Undefined();
}

JsValue Disable(std::shared_ptr<PartOne> partOne,
                const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  obj.Disable();

  return JsValue::Undefined();
}

JsValue DoReloot(std::shared_ptr<PartOne> partOne,
                 const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  obj.DoReloot();

  return JsValue::Undefined();
}

JsValue Enable(std::shared_ptr<PartOne> partOne,
               const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  obj.Enable();

  return JsValue::Undefined();
}

JsValue GetAngle(std::shared_ptr<PartOne> partOne,
                 const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  auto niPoint3 = obj.GetAngle();
  auto arr = JsValue::Array(3);
  for (int i = 0; i < 3; ++i) {
    arr.SetProperty(JsValue(i), JsValue(niPoint3[i]));
  }

  return arr;
}

JsValue GetAnimationVariableBool(std::shared_ptr<PartOne> partOne,
                                 const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto animation = args[1].ToString();
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  return JsValue::Bool(obj.GetAnimationVariableBool(animation.c_str()));
}

JsValue GetBaseId(std::shared_ptr<PartOne> partOne,
                  const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  return JsValue::Int(obj.GetBaseId());
}

JsValue GetCellOrWorld(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  auto espmFileNames = partOne->GetEspm().GetFileNames();

  return JsValue::Int(obj.GetCellOrWorld().ToFormId(espmFileNames));
}

JsValue GetInventory(std::shared_ptr<PartOne> partOne,
                     const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  auto espmFileNames = partOne->GetEspm().GetFileNames();

  auto inventory = obj.GetInventory();

  auto jsInventory = JsValue::Object();

  auto entries = JsValue::Array(inventory.GetTotalItemCount());

  for (int i = 0; i < inventory.entries.size(); i++) {
    auto item = JsValue::Object();

    item.SetProperty("baseId", JsValue::Int(inventory.entries[i].baseId));
    item.SetProperty("count", JsValue::Int(inventory.entries[i].count));

    entries.SetProperty(i, item);
  }

  jsInventory.SetProperty("entries", entries);

  return jsInventory;
}

JsValue GetPos(std::shared_ptr<PartOne> partOne,
               const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  auto niPoint3 = obj.GetPos();
  auto arr = JsValue::Array(3);
  for (int i = 0; i < 3; ++i) {
    arr.SetProperty(JsValue(i), JsValue(niPoint3[i]));
  }

  return arr;
}

JsValue IsActivationBlocked(std::shared_ptr<PartOne> partOne,
                            const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  return JsValue::Bool(obj.IsActivationBlocked());
}

JsValue IsDisabled(std::shared_ptr<PartOne> partOne,
                   const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  return JsValue::Bool(obj.IsDisabled());
}

JsValue IsHarvested(std::shared_ptr<PartOne> partOne,
                    const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  return JsValue::Bool(obj.IsHarvested());
}

JsValue IsOpen(std::shared_ptr<PartOne> partOne,
               const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  return JsValue::Bool(obj.IsOpen());
}

JsValue RemoveAllItems(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  auto formId2 = Uint32FromJsValue(args[1].GetProperty("_formId"));
  if (formId2 != 0) {
    auto& obj2 = partOne->worldState.GetFormAt<MpObjectReference>(formId);
    obj.RemoveAllItems(&obj2);
    return JsValue::Undefined();
  }

  obj.RemoveAllItems();

  return JsValue::Undefined();
}

JsValue RemoveItem(std::shared_ptr<PartOne> partOne,
                   const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  auto baseId = Uint32FromJsValue(args[1]);
  auto count = Uint32FromJsValue(args[2]);

  if (count == 0) {
    count = 1;
  }

  auto formId2 = Uint32FromJsValue(args[3]);
  if (formId2 != 0) {
    auto& obj2 = partOne->worldState.GetFormAt<MpObjectReference>(formId);
    obj.RemoveItem(baseId, count, &obj2);
    return JsValue::Undefined();
  }
  obj.RemoveItem(baseId, count, nullptr);

  return JsValue::Undefined();
}

JsValue SetActivationBlocked(std::shared_ptr<PartOne> partOne,
                             const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  auto activationBlocked = args[1];

  obj.SetActivationBlocked(args[1]);

  return JsValue::Undefined();
}

JsValue SetAngle(std::shared_ptr<PartOne> partOne,
                 const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  double x = static_cast<double>(args[1]);
  double y = static_cast<double>(args[2]);
  double z = static_cast<double>(args[3]);

  NiPoint3 niPoint3(x, y, z);

  obj.SetAngle(niPoint3);

  return JsValue::Undefined();
}

JsValue SetAnimationVariableBool(std::shared_ptr<PartOne> partOne,
                                 const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  std::string name = args[1].ToString();
  bool boolValue = args[2];

  obj.SetAnimationVariableBool(name.c_str(), boolValue);

  return JsValue::Undefined();
}

JsValue SetCellOrWorld(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  const auto espmFileNames = partOne->worldState.GetEspm().GetFileNames();

  obj.SetCellOrWorld(
    FormDesc::FromFormId(Uint32FromJsValue(args[1]), espmFileNames));

  return JsValue::Undefined();
}

JsValue SetHarvested(std::shared_ptr<PartOne> partOne,
                     const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  obj.SetHarvested(args[1]);

  return JsValue::Undefined();
}

JsValue SetPos(std::shared_ptr<PartOne> partOne,
               const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto& obj = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  double x = static_cast<double>(args[1]);
  double y = static_cast<double>(args[2]);
  double z = static_cast<double>(args[3]);

  NiPoint3 niPoint3(x, y, z);

  obj.SetPos(niPoint3);

  return JsValue::Undefined();
}
