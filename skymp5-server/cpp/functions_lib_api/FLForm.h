#pragma once
#include "PartOne.h"
#include <JsEngine.h>

void RegisterFormApi(std::shared_ptr<PartOne> partOne);

JsValue FormCtor(std::shared_ptr<PartOne> partOne,
                 const JsFunctionArguments& args);

JsValue GetFormId(const JsFunctionArguments& args);

JsValue GetName(std::shared_ptr<PartOne> partOne,
                const JsFunctionArguments& args);

JsValue GetGoldValue(std::shared_ptr<PartOne> partOne,
                     const JsFunctionArguments& args);

JsValue GetWeight(std::shared_ptr<PartOne> partOne,
                  const JsFunctionArguments& args);

JsValue GetKeywords(std::shared_ptr<PartOne> partOne,
                    const JsFunctionArguments& args);

JsValue GetNthKeyword(std::shared_ptr<PartOne> partOne,
                      const JsFunctionArguments& args);

JsValue GetNumKeywords(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args);

JsValue HasKeyword(std::shared_ptr<PartOne> partOne,
                   const JsFunctionArguments& args);

JsValue GetType(std::shared_ptr<PartOne> partOne,
                const JsFunctionArguments& args);

JsValue GetEditorId(std::shared_ptr<PartOne> partOne,
                    const JsFunctionArguments& args);

JsValue EqualSignature(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args);