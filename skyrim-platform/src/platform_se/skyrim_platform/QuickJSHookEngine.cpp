#include "QuickJSHookEngine.h"
#include "HooksStorage.h"
#include <map>
#include <quickjs.h>
#include <spdlog/spdlog.h>
#include <string>

namespace {

std::string GetQJSError(JSContext* ctx)
{
  JSValue exception = JS_GetException(ctx);
  const char* str = JS_ToCString(ctx, exception);
  std::string result = str ? str : "(unknown error)";
  JS_FreeCString(ctx, str);

  // Try to get stack trace
  if (JS_IsError(exception)) {
    JSValue stack = JS_GetPropertyStr(ctx, exception, "stack");
    if (!JS_IsUndefined(stack)) {
      const char* stackStr = JS_ToCString(ctx, stack);
      if (stackStr) {
        result += "\n";
        result += stackStr;
        JS_FreeCString(ctx, stackStr);
      }
    }
    JS_FreeValue(ctx, stack);
  }

  JS_FreeValue(ctx, exception);
  return result;
}

// Convert HooksStorage::Value to JSValue
JSValue StorageValueToJS(JSContext* ctx, const HooksStorage::Value& val)
{
  if (std::holds_alternative<std::monostate>(val)) {
    return JS_UNDEFINED;
  }
  if (std::holds_alternative<bool>(val)) {
    return JS_NewBool(ctx, std::get<bool>(val));
  }
  if (std::holds_alternative<double>(val)) {
    return JS_NewFloat64(ctx, std::get<double>(val));
  }
  if (std::holds_alternative<std::string>(val)) {
    const auto& s = std::get<std::string>(val);
    return JS_NewStringLen(ctx, s.c_str(), s.size());
  }
  return JS_UNDEFINED;
}

// Convert JSValue to HooksStorage::Value
HooksStorage::Value JSToStorageValue(JSContext* ctx, JSValue val)
{
  if (JS_IsBool(val)) {
    return static_cast<bool>(JS_ToBool(ctx, val));
  }
  if (JS_IsNumber(val)) {
    double d;
    JS_ToFloat64(ctx, &d, val);
    return d;
  }
  if (JS_IsString(val)) {
    const char* s = JS_ToCString(ctx, val);
    std::string result = s ? s : "";
    JS_FreeCString(ctx, s);
    return result;
  }
  return std::monostate{};
}

// --- hooksStorage JS bindings ---

JSValue JS_HooksStorage_Get(JSContext* ctx, JSValueConst /*thisVal*/, int argc,
                            JSValueConst* argv)
{
  if (argc < 1) {
    return JS_UNDEFINED;
  }
  const char* key = JS_ToCString(ctx, argv[0]);
  if (!key) {
    return JS_UNDEFINED;
  }
  auto val = HooksStorage::GetSingleton().Get(key);
  JS_FreeCString(ctx, key);
  return StorageValueToJS(ctx, val);
}

JSValue JS_HooksStorage_Set(JSContext* ctx, JSValueConst /*thisVal*/, int argc,
                            JSValueConst* argv)
{
  if (argc < 2) {
    return JS_UNDEFINED;
  }
  const char* key = JS_ToCString(ctx, argv[0]);
  if (!key) {
    return JS_UNDEFINED;
  }
  auto val = JSToStorageValue(ctx, argv[1]);
  HooksStorage::GetSingleton().Set(key, std::move(val));
  JS_FreeCString(ctx, key);
  return JS_UNDEFINED;
}

JSValue JS_HooksStorage_Has(JSContext* ctx, JSValueConst /*thisVal*/, int argc,
                            JSValueConst* argv)
{
  if (argc < 1) {
    return JS_FALSE;
  }
  const char* key = JS_ToCString(ctx, argv[0]);
  if (!key) {
    return JS_FALSE;
  }
  bool has = HooksStorage::GetSingleton().Has(key);
  JS_FreeCString(ctx, key);
  return JS_NewBool(ctx, has);
}

JSValue JS_HooksStorage_Erase(JSContext* ctx, JSValueConst /*thisVal*/,
                              int argc, JSValueConst* argv)
{
  if (argc < 1) {
    return JS_UNDEFINED;
  }
  const char* key = JS_ToCString(ctx, argv[0]);
  if (!key) {
    return JS_UNDEFINED;
  }
  HooksStorage::GetSingleton().Erase(key);
  JS_FreeCString(ctx, key);
  return JS_UNDEFINED;
}

JSValue JS_HooksStorage_Clear(JSContext* ctx, JSValueConst /*thisVal*/,
                              int /*argc*/, JSValueConst* /*argv*/)
{
  HooksStorage::GetSingleton().Clear();
  return JS_UNDEFINED;
}

void InstallHooksStorageBindings(JSContext* ctx)
{
  JSValue global = JS_GetGlobalObject(ctx);

  JSValue storage = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, storage, "get",
                    JS_NewCFunction(ctx, JS_HooksStorage_Get, "get", 1));
  JS_SetPropertyStr(ctx, storage, "set",
                    JS_NewCFunction(ctx, JS_HooksStorage_Set, "set", 2));
  JS_SetPropertyStr(ctx, storage, "has",
                    JS_NewCFunction(ctx, JS_HooksStorage_Has, "has", 1));
  JS_SetPropertyStr(ctx, storage, "erase",
                    JS_NewCFunction(ctx, JS_HooksStorage_Erase, "erase", 1));
  JS_SetPropertyStr(ctx, storage, "clear",
                    JS_NewCFunction(ctx, JS_HooksStorage_Clear, "clear", 0));

  JS_SetPropertyStr(ctx, global, "hooksStorage", storage);
  JS_FreeValue(ctx, global);
}

} // anonymous namespace

struct QuickJSHookEngine::Impl
{
  JSRuntime* rt = nullptr;
  JSContext* ctx = nullptr;

  struct CompiledScript
  {
    // We store compiled bytecode so we don't re-parse every call
    bool hasEnter = false;
    bool hasLeave = false;
  };

  std::map<uint32_t, CompiledScript> scripts;
  uint32_t nextId = 1;
};

QuickJSHookEngine::QuickJSHookEngine()
  : pImpl(std::make_unique<Impl>())
{
  pImpl->rt = JS_NewRuntime();
  pImpl->ctx = JS_NewContext(pImpl->rt);

  InstallHooksStorageBindings(pImpl->ctx);
}

QuickJSHookEngine::~QuickJSHookEngine()
{
  if (pImpl->ctx) {
    JS_FreeContext(pImpl->ctx);
  }
  if (pImpl->rt) {
    JS_FreeRuntime(pImpl->rt);
  }
}

uint32_t QuickJSHookEngine::Compile(const std::string& source,
                                    const std::string& filename,
                                    std::string& outError)
{
  // We wrap the user source so that enter() and leave() are defined as
  // properties on a per-script namespace object. The user script should define
  // function enter(ctx) { ... } and/or function leave(ctx) { ... }.
  //
  // We store them as global __hook_N_enter / __hook_N_leave.
  uint32_t id = pImpl->nextId++;
  std::string prefix = "__hook_" + std::to_string(id);

  // Wrap in an IIFE that captures enter/leave into globals
  std::string wrapped = "(function() {\n" + source +
    "\nif (typeof enter === 'function') globalThis." + prefix +
    "_enter = enter;\n"
    "if (typeof leave === 'function') globalThis." +
    prefix +
    "_leave = leave;\n"
    "})();\n";

  JSValue result = JS_Eval(pImpl->ctx, wrapped.c_str(), wrapped.size(),
                           filename.c_str(), JS_EVAL_TYPE_GLOBAL);

  if (JS_IsException(result)) {
    outError = GetQJSError(pImpl->ctx);
    JS_FreeValue(pImpl->ctx, result);
    pImpl->nextId--; // reclaim the ID
    return 0;
  }
  JS_FreeValue(pImpl->ctx, result);

  // Check which functions were defined
  Impl::CompiledScript cs;
  JSValue global = JS_GetGlobalObject(pImpl->ctx);

  JSValue enterFn =
    JS_GetPropertyStr(pImpl->ctx, global, (prefix + "_enter").c_str());
  cs.hasEnter = JS_IsFunction(pImpl->ctx, enterFn);
  JS_FreeValue(pImpl->ctx, enterFn);

  JSValue leaveFn =
    JS_GetPropertyStr(pImpl->ctx, global, (prefix + "_leave").c_str());
  cs.hasLeave = JS_IsFunction(pImpl->ctx, leaveFn);
  JS_FreeValue(pImpl->ctx, leaveFn);

  JS_FreeValue(pImpl->ctx, global);

  if (!cs.hasEnter && !cs.hasLeave) {
    outError = "Script must define at least one of: enter(ctx), leave(ctx)";
    return 0;
  }

  pImpl->scripts[id] = cs;
  return id;
}

QuickJSHookEngine::EnterResult QuickJSHookEngine::RunEnter(
  uint32_t scriptId, uint32_t selfId, const std::string& eventName)
{
  EnterResult result;
  result.eventName = eventName;

  auto it = pImpl->scripts.find(scriptId);
  if (it == pImpl->scripts.end()) {
    result.error = "Script not found";
    return result;
  }

  if (!it->second.hasEnter) {
    result.ok = true;
    return result;
  }

  std::string prefix = "__hook_" + std::to_string(scriptId);
  JSContext* ctx = pImpl->ctx;

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue enterFn =
    JS_GetPropertyStr(ctx, global, (prefix + "_enter").c_str());

  // Build context object: { selfId, eventName }
  JSValue ctxObj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, ctxObj, "selfId",
                    JS_NewFloat64(ctx, static_cast<double>(selfId)));
  JS_SetPropertyStr(ctx, ctxObj, "eventName",
                    JS_NewStringLen(ctx, eventName.c_str(), eventName.size()));

  JSValue ret = JS_Call(ctx, enterFn, JS_UNDEFINED, 1, &ctxObj);

  if (JS_IsException(ret)) {
    result.error = GetQJSError(ctx);
  } else {
    result.ok = true;
    // Read back eventName from context (script may have modified it)
    JSValue updatedName = JS_GetPropertyStr(ctx, ctxObj, "eventName");
    if (JS_IsString(updatedName)) {
      const char* s = JS_ToCString(ctx, updatedName);
      if (s) {
        result.eventName = s;
        JS_FreeCString(ctx, s);
      }
    }
    JS_FreeValue(ctx, updatedName);
  }

  JS_FreeValue(ctx, ret);
  JS_FreeValue(ctx, ctxObj);
  JS_FreeValue(ctx, enterFn);
  JS_FreeValue(ctx, global);

  return result;
}

QuickJSHookEngine::LeaveResult QuickJSHookEngine::RunLeave(uint32_t scriptId,
                                                           bool succeeded)
{
  LeaveResult result;

  auto it = pImpl->scripts.find(scriptId);
  if (it == pImpl->scripts.end()) {
    result.error = "Script not found";
    return result;
  }

  if (!it->second.hasLeave) {
    result.ok = true;
    return result;
  }

  std::string prefix = "__hook_" + std::to_string(scriptId);
  JSContext* ctx = pImpl->ctx;

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue leaveFn =
    JS_GetPropertyStr(ctx, global, (prefix + "_leave").c_str());

  JSValue ctxObj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, ctxObj, "succeeded", JS_NewBool(ctx, succeeded));

  JSValue ret = JS_Call(ctx, leaveFn, JS_UNDEFINED, 1, &ctxObj);

  if (JS_IsException(ret)) {
    result.error = GetQJSError(ctx);
  } else {
    result.ok = true;
  }

  JS_FreeValue(ctx, ret);
  JS_FreeValue(ctx, ctxObj);
  JS_FreeValue(ctx, leaveFn);
  JS_FreeValue(ctx, global);

  return result;
}

void QuickJSHookEngine::RemoveScript(uint32_t scriptId)
{
  auto it = pImpl->scripts.find(scriptId);
  if (it == pImpl->scripts.end()) {
    return;
  }

  std::string prefix = "__hook_" + std::to_string(scriptId);
  JSContext* ctx = pImpl->ctx;
  JSValue global = JS_GetGlobalObject(ctx);

  // Delete the global functions
  JSAtom enterAtom = JS_NewAtom(ctx, (prefix + "_enter").c_str());
  JS_DeleteProperty(ctx, global, enterAtom, 0);
  JS_FreeAtom(ctx, enterAtom);

  JSAtom leaveAtom = JS_NewAtom(ctx, (prefix + "_leave").c_str());
  JS_DeleteProperty(ctx, global, leaveAtom, 0);
  JS_FreeAtom(ctx, leaveAtom);

  JS_FreeValue(ctx, global);
  pImpl->scripts.erase(it);
}
