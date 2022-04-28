#include "FLForm.h"
#include "FormDesc.h"
#include "FunctionsLibApi.h"

void RegisterFormApi(std::shared_ptr<PartOne> partOne)
{
  JsValue globalObject = JsValue::GlobalObject();

  // Form ctor
  globalObject.SetProperty(
    "Form", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return FormCtor(partOne, args);
    }));

  JsValue form = globalObject.GetProperty("Form");
  JsValue formPrototype = form.GetProperty("prototype");

  formPrototype.SetProperty("GetFormId", JsValue::Function(GetFormId));
  formPrototype.SetProperty(
    "GetName", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetName(partOne, args);
    }));
}

JsValue FormCtor(std::shared_ptr<PartOne> partOne,
                 const JsFunctionArguments& args)
{
  auto formId = FormIdFromJsValue(args[1]);
  if (formId == 0) {
    // GetLogger()->error("Error on check formId");
    return JsValue::Undefined();
  }

  args[0].SetProperty("_formId", args[1]);

  if (formId >= 0xff000000) {
    return JsValue::Undefined();
  }

  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);

  if (!lookupRes.rec || lookupRes.rec->GetType().ToString() == "") {
    // GetLogger()->error("Form not exists");
    return JsValue::Undefined();
  }

  return JsValue::Undefined();
}

JsValue GetFormId(const JsFunctionArguments& args)
{
  return args[0].GetProperty("_formId");
}

JsValue GetName(std::shared_ptr<PartOne> partOne,
                const JsFunctionArguments& args)
{
  auto formId = FormIdFromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);
  auto& cache = partOne->worldState.GetEspmCache();

  espm::IterateFields_(
    lookupRes.rec,
    [&](const char* type, uint32_t size, const char* data) {
      if (std::string(type, 4) == "FULL") {
        // std::cout << data;
        // std::cout << "\n";
        std::cout << *reinterpret_cast<const uint32_t*>(data);
        std::cout << "\n";
        // if (size == 4) {
        //   // JsValue arr = JsValue::Uint8Array(size);
        //   // memcpy(arr.GetTypedArrayData(), data, size);

        //   //
        //   JsValue::GlobalObject().GetProperty("form").SetProperty("_test",
        //   //                                                         arr);

        //   // return arr;

        //   // spdlog::get("console")->info("data");
        //   // GetLogger()->error("TODO: use localization files in GetName");
        //   // std::cout << "\n";
        //   // std::cout << (*reinterpret_cast<const uint32_t*>(data));
        //   // std::cout << (*reinterpret_cast<const uint32_t*>(data));
        //   // std::cout << "\n";
        //   // std::cout << "\n";
        //   std::stringstream ss;
        //   ss << *reinterpret_cast<const uint32_t*>(data);
        //   std::string str;
        //   ss >> str;

        //   return JsValue::String(str);
        // }

        // std::cout << "\n";

        return JsValue::String(data);
      }
    },

    cache);

  return JsValue::Undefined();
}
