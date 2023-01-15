#include <functional>

class JsFunctionArguments;
class JsValue;

using FunctionT = std::function<JsValue(const JsFunctionArguments& args)>;
