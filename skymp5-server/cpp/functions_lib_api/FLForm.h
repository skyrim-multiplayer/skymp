#include "PartOne.h"
#include <JsEngine.h>

void RegisterFormApi(std::shared_ptr<PartOne> partOne);

JsValue FormCtor(std::shared_ptr<PartOne> partOne,
                 const JsFunctionArguments& args);
JsValue GetFormId(const JsFunctionArguments& args);
JsValue GetName(std::shared_ptr<PartOne> partOne,
                const JsFunctionArguments& args);
