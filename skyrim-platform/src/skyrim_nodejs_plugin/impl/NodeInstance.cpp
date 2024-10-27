#include <iostream>
#include <libplatform/libplatform.h>
#include <map>
#include <memory>
#include <node.h>
#include <string>
#include <uv.h>
#include <v8.h>
#include <vector>

#include <env-inl.h>
#include <env.h>

#include "NodeInstance.h"

using namespace node;
using namespace v8;

namespace {
std::string g_javaScriptError;

void ReportErrorCallback(const FunctionCallbackInfo<Value>& args)
{
  Isolate* isolate = args.GetIsolate();

  if (args.Length() < 1) {
    return;
  }

  String::Utf8Value error(isolate, args[0]);
  std::string s = *error ? *error : "Unknown JavaScript error";

  g_javaScriptError = s;
}

void RegisterReportError(Isolate* isolate, Local<Context> context)
{
  context->Global()->Set(
    context,
    String::NewFromUtf8(isolate, "reportError", NewStringType::kNormal).ToLocalChecked(),
    FunctionTemplate::New(isolate, ReportErrorCallback)->GetFunction(context).ToLocalChecked()
  ).ToChecked();
}
}

struct NodeInstance::Impl
{
  std::map<void*, std::shared_ptr<Isolate::CreateParams>> createParamsMap;
  std::map<void*, Isolate*> isolatesMap;
  std::map<void*, v8::Persistent<v8::Context>> contextsMap;
  std::unique_ptr<MultiIsolatePlatform> platform;
  std::string error;
};

NodeInstance::NodeInstance()
{
  pImpl = std::make_shared<Impl>();
}

void NodeInstance::Load()
{
  // Do nothing
}

int NodeInstance::Init(int argc, char** argv)
{
  std::vector<std::string> args(argv, argv + argc);
  std::shared_ptr<node::InitializationResult> result =
    node::InitializeOncePerProcess(
      args,
      {
        node::ProcessInitializationFlags::kNoInitializeV8,
        node::ProcessInitializationFlags::kNoInitializeNodeV8Platform,
        // This is used to test NODE_REPL_EXTERNAL_MODULE is disabled with
        // kDisableNodeOptionsEnv. If other tests need NODE_OPTIONS
        // support in the future, split this configuration out as a
        // command line option.
        node::ProcessInitializationFlags::kDisableNodeOptionsEnv,
      });

  for (const std::string& error : result->errors())
    fprintf(stderr, "%s: %s\n", args[0].c_str(), error.c_str());
  if (result->early_return() != 0) {
    return result->exit_code();
  }

  // TODO: put here global part of CreateEnvironment
  // TODO: global deinit with V8::DisposePlatform();
  // node::TearDownOncePerProcess();

  return 0;
}

int NodeInstance::CreateEnvironment(int argc, char** argv, void** outEnv)
{
  // Create a v8::Platform instance. `MultiIsolatePlatform::Create()` is a way
  // to create a v8::Platform instance that Node.js can use when creating
  // Worker threads. When no `MultiIsolatePlatform` instance is present,
  // Worker threads are disabled.
  pImpl->platform = MultiIsolatePlatform::Create(4);
  V8::InitializePlatform(pImpl->platform.get());
  V8::Initialize();

  // Setup V8 isolate and context
  // auto create_params = std::make_shared<Isolate::CreateParams>();
  // create_params->array_buffer_allocator =
  //   v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  // Isolate* isolate = Isolate::New(*create_params);
  Isolate* isolate = Isolate::Allocate();
  pImpl->platform->RegisterIsolate(isolate, uv_default_loop());
  auto create_params = std::make_shared<Isolate::CreateParams>();

  std::shared_ptr<node::ArrayBufferAllocator> allocator =
    node::ArrayBufferAllocator::Create();
  isolate = NewIsolate(allocator, uv_default_loop(), pImpl->platform.get());

  // register the isolate with the platform
  // platform->RegisterIsolate(isolate, uv_default_loop());

  {
    // Setup scope and context
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    Local<Context> context = Context::New(isolate);

    RegisterReportError(isolate, context);

    Context::Scope context_scope(context);

    // Initialize node environment
    node::IsolateData* isolate_data = node::CreateIsolateData(
      isolate, uv_default_loop(), pImpl->platform.get());

    std::vector<std::string> args(argv, argv + argc);
    node::Environment* env = node::CreateEnvironment(
      isolate_data, context, args, std::vector<std::string>());

    node::LoadEnvironment(env,
                          "const publicRequire = "
                          "require('node:module').createRequire(process.cwd() "
                          "+ '/'); globalThis.require = publicRequire;",
                          nullptr);

    pImpl->createParamsMap[env] = create_params;
    pImpl->contextsMap[env].Reset(isolate,
                                  context); // Promote to Persistent and store
    pImpl->isolatesMap[env] = isolate;

    // Store the environment for later use
    *outEnv = static_cast<void*>(env);
  }

  pImpl->error = "Success";
  return 0;
}

int NodeInstance::DestroyEnvironment(void* env)
{
  if (!env) {
    pImpl->error = "No env";
    return -1;
  }

  auto& context = pImpl->contextsMap[env];
  auto& isolate = pImpl->isolatesMap[env];

  // Step 1: Dispose of the V8 context (Persistent)
  if (!context.IsEmpty()) {
    context.Reset(); // Reset the Persistent handle to free the context
  }

  // Step 2: Dispose of the V8 isolate
  if (isolate) {
    isolate->Dispose(); // Clean up the V8 isolate
    isolate = nullptr;  // Null out the reference to avoid dangling pointers
  }

  // Step 3: Optionally close the libuv loop if you're using one
  // For example:
  // uv_loop_close(uv_default_loop());  // Only if you created your own loop

  auto create_params = pImpl->createParamsMap[env];

  if (create_params) {
    delete create_params->array_buffer_allocator;
  }

  pImpl->createParamsMap.erase(env);
  pImpl->contextsMap.erase(env);
  pImpl->isolatesMap.erase(env);

  pImpl->error = "Success";
  return 0;
}

int NodeInstance::Tick(void* env)
{
  if (!env) {
    pImpl->error = "No environment";
    return -1;
  }

  uv_run(uv_default_loop(), UV_RUN_NOWAIT);

  // Process any microtasks (e.g., resolved Promises)
  auto isolate = pImpl->isolatesMap[env];
  if (!isolate) {
    pImpl->error = "No isolate";
    return -1;
  }

  isolate->PerformMicrotaskCheckpoint();

  return 0; // Success
}

int NodeInstance::ExecuteScript(void* env, const char* script)
{
  if (!env) {
    pImpl->error = "No env";
    return -1;
  }

  if (!script) {
    pImpl->error = "No script";
    return -1;
  }

  Isolate* isolate = pImpl->isolatesMap[env];
  if (!isolate) {
    pImpl->error = "No isolate";
    return -1;
  }

  Isolate::Scope isolate_scope(isolate);
  HandleScope handle_scope(isolate);

  auto& contextPersistent = pImpl->contextsMap[env];
  Local<Context> context = contextPersistent.Get(isolate);
  Context::Scope context_scope(context);

  Local<String> source = String::NewFromUtf8(isolate, script).ToLocalChecked();
  Local<Script> compiled_script;

  // Use TryCatch to handle any exceptions that might occur
  TryCatch try_catch(isolate);

  if (!Script::Compile(context, source).ToLocal(&compiled_script)) {
    String::Utf8Value error(isolate, try_catch.Exception());
    pImpl->error = *error ? *error : "Unknown compilation error";
    return -1;
  }

  // Execute script and catch potential runtime exceptions
  if (!compiled_script->Run(context).IsEmpty()) {
    Local<Value> result;
    if (!compiled_script->Run(context).ToLocal(&result)) {
      String::Utf8Value error(isolate, try_catch.Exception());
      pImpl->error = *error ? *error : "Unknown runtime error";
      return -1;
    }
  } else {
    String::Utf8Value error(isolate, try_catch.Exception());
    pImpl->error = *error ? *error : "Unknown runtime error";
    return -1;
  }

  pImpl->error = "Success";
  return 0;
}

uint64_t NodeInstance::GetError(char* buffer, uint64_t bufferSize)
{
  constexpr size_t kNullTerminatorLengthInBytes = 1;

  if (bufferSize > 0) {
    size_t copyLength =
      std::min(static_cast<size_t>(bufferSize - kNullTerminatorLengthInBytes),
               pImpl->error.size());

    std::memcpy(buffer, pImpl->error.data(), copyLength);

    buffer[copyLength] = '\0';
  }

  return pImpl->error.size() + kNullTerminatorLengthInBytes;
}

const char* NodeInstance::GetJavaScriptError()
{
  return g_javaScriptError.c_str();
}

void NodeInstance::ClearJavaScriptError()
{
  g_javaScriptError.clear();
}
