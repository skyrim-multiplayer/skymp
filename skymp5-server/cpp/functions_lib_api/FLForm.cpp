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

  formPrototype.SetProperty(
    "GetGoldValue",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetGoldValue(partOne, args);
    }));

  formPrototype.SetProperty(
    "GetGoldValue",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetGoldValue(partOne, args);
    }));

  formPrototype.SetProperty(
    "GetWeight", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetWeight(partOne, args);
    }));

  formPrototype.SetProperty(
    "GetKeywords",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetKeywords(partOne, args);
    }));

  formPrototype.SetProperty(
    "GetNthKeyword",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetNthKeyword(partOne, args);
    }));

  formPrototype.SetProperty(
    "GetNumKeywords",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetNumKeywords(partOne, args);
    }));

  formPrototype.SetProperty(
    "HasKeyword",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return HasKeyword(partOne, args);
    }));

  formPrototype.SetProperty(
    "GetType", JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetType(partOne, args);
    }));

  formPrototype.SetProperty(
    "GetEditorId",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetEditorId(partOne, args);
    }));

  formPrototype.SetProperty(
    "GetSignature",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return GetType(partOne, args);
    }));

  formPrototype.SetProperty(
    "EqualSignature",
    JsValue::Function([partOne](const JsFunctionArguments& args) {
      return EqualSignature(partOne, args);
    }));
}

JsValue FormCtor(std::shared_ptr<PartOne> partOne,
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

  if (!lookupRes.rec || lookupRes.rec->GetType().ToString() == "") {
    partOne->GetLogger().error("Form not exists");
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
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);
  auto& cache = partOne->worldState.GetEspmCache();

  espm::IterateFields_(
    lookupRes.rec,
    [&](const char* type, uint32_t size, const char* data) {
      if (std::string(type, 4) == "FULL") {
        throw std::runtime_error(
          "Form.GetName() is WIP (need localizationProvider)");
      }
    },
    cache);

  return JsValue::Undefined();
}

JsValue GetGoldValue(std::shared_ptr<PartOne> partOne,
                     const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);
  auto& cache = partOne->worldState.GetEspmCache();

  JsValue goldValue = JsValue::Undefined();

  espm::IterateFields_(
    lookupRes.rec,
    [&](const char* type, uint32_t size, const char* data) {
      if (std::string(type, 4) == "DATA") {
        goldValue = JsValue::Int(*(uint32_t*)data);
      }
    },
    cache);

  return goldValue;
}

JsValue GetWeight(std::shared_ptr<PartOne> partOne,
                  const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);
  auto& cache = partOne->worldState.GetEspmCache();

  JsValue weight = JsValue::Undefined();

  espm::IterateFields_(
    lookupRes.rec,
    [&](const char* type, uint32_t size, const char* data) {
      if (std::string(type, 4) == "DATA") {
        weight = JsValue::Double(*(float*)&data[4]);
      }
    },
    cache);

  return weight;
}

JsValue GetKeywords(std::shared_ptr<PartOne> partOne,
                    const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);
  auto& cache = partOne->worldState.GetEspmCache();

  std::vector<uint32_t> keywords;

  espm::IterateFields_(
    lookupRes.rec,
    [&](const char* type, uint32_t size, const char* data) {
      if (std::string(type, 4) == "KWDA") {
        keywords.resize(size / 4);
        for (int i = 0; i < size; i += 4) {
          keywords[i] = *(uint32_t*)data;
        }
      }
    },
    cache);

  JsValue jsKeywords = JsValue::Array(keywords.size());

  for (int i = 0; i < keywords.size(); i++) {
    jsKeywords.SetProperty(i, JsValue::Int(keywords[i]));
  }

  return jsKeywords;
}

JsValue GetNthKeyword(std::shared_ptr<PartOne> partOne,
                      const JsFunctionArguments& args)
{
  return GetKeywords(partOne, args).GetProperty(args[1]);
}

JsValue GetNumKeywords(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);
  auto& cache = partOne->worldState.GetEspmCache();

  JsValue numKeywords = JsValue::Int(0);

  espm::IterateFields_(
    lookupRes.rec,
    [&](const char* type, uint32_t size, const char* data) {
      if (std::string(type, 4) == "KSIZ") {
        numKeywords = JsValue::Int(*(uint32_t*)data);
      }
    },
    cache);

  return numKeywords;
}

JsValue HasKeyword(std::shared_ptr<PartOne> partOne,
                   const JsFunctionArguments& args)
{
  uint32_t keywordId = Uint32FromJsValue(args[1]);
  JsValue keywords = GetKeywords(partOne, args);

  for (int i = 0; i < int(keywords.GetProperty("length")); i++) {
    if (Uint32FromJsValue(keywords.GetProperty(i)) == keywordId) {
      return JsValue::Bool(true);
    }
  }

  return JsValue::Bool(false);
}

JsValue GetType(std::shared_ptr<PartOne> partOne,
                const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);

  return JsValue::String(lookupRes.rec->GetType().ToString());
}

JsValue GetEditorId(std::shared_ptr<PartOne> partOne,
                    const JsFunctionArguments& args)
{
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);
  auto& cache = partOne->worldState.GetEspmCache();

  JsValue editorId = JsValue::Undefined();

  espm::IterateFields_(
    lookupRes.rec,
    [&](const char* type, uint32_t size, const char* data) {
      if (std::string(type, 4) == "NAME") {
        uint32_t baseId = *(uint32_t*)data;
        auto lookupRes =
          partOne->GetEspm().GetBrowser().LookupById(baseId).rec->GetEditorId(
            cache);
      }
    },
    cache);

  if (editorId.GetType() != JsValue::Type::Undefined) {
    return editorId;
  }

  return JsValue::String(lookupRes.rec->GetEditorId(cache));
}

JsValue EqualSignature(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args)
{
  std::string signature = args[1].ToString();
  auto formId = Uint32FromJsValue(args[0].GetProperty("_formId"));
  auto lookupRes = partOne->GetEspm().GetBrowser().LookupById(formId);

  return JsValue::Bool(signature == lookupRes.rec->GetType().ToString());
}
