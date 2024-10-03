#include <iostream>
#include <libplatform/libplatform.h>
#include <memory>
#include <node.h>
#include <string>
#include <uv.h>
#include <v8.h>
#include <vector>
#include <map>

#include <env.h>
#include <env-inl.h>

#include "NodeInstance.h"

using namespace node;
using namespace v8;

struct NodeInstance::Impl
{
  std::map<void *, std::shared_ptr<Isolate::CreateParams>> createParamsMap;
  std::map<void *, Isolate*> isolatesMap;
  std::map<void *, v8::Persistent<v8::Context>> contextsMap;
};

NodeInstance::NodeInstance()
{
  pImpl = std::make_shared<Impl>();
}

void NodeInstance::Load()
{
  // Do nothing
}

int NodeInstance::Init()
{
  // TODO: put here global part of CreateEnvironment
  // TODO: global deinit with V8::DisposePlatform(); node::TearDownOncePerProcess();
}

int NodeInstance::CreateEnvironment(int argc, char** argv, void** outEnv)
{
  // Create a v8::Platform instance. `MultiIsolatePlatform::Create()` is a way
  // to create a v8::Platform instance that Node.js can use when creating
  // Worker threads. When no `MultiIsolatePlatform` instance is present,
  // Worker threads are disabled.
  std::unique_ptr<MultiIsolatePlatform> platform =
    MultiIsolatePlatform::Create(4);
  V8::InitializePlatform(platform.get());
  V8::Initialize();

  // Setup V8 isolate and context
  auto create_params = std::make_shared<Isolate::CreateParams>();
  create_params->array_buffer_allocator =
    v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  Isolate* isolate = Isolate::New(*create_params);

  {
    // Setup scope and context
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    Local<Context> context = Context::New(isolate);
    Context::Scope context_scope(context);

    // Initialize node environment
    node::IsolateData* isolate_data =
      node::CreateIsolateData(isolate, uv_default_loop(), platform.get());

    std::vector<std::string> args(argv, argv + argc);
    node::Environment* env = node::CreateEnvironment(
      isolate_data, context, args, std::vector<std::string>());

    pImpl->createParamsMap[env] = create_params;
    pImpl->contextsMap[env].Reset(isolate, context); // Promote to Persistent and store
    pImpl->isolatesMap[env] = isolate;

    // Store the environment for later use
    *outEnv = static_cast<void*>(env);
  }

  return 0; // Success
}

int NodeInstance::DestroyEnvironment(void* env)
{
  if (env) {
    auto &context = pImpl->contextsMap[env];
    auto &isolate = pImpl->isolatesMap[env];

    // Step 1: Dispose of the V8 context (Persistent)
    if (!context.IsEmpty()) {
        context.Reset();  // Reset the Persistent handle to free the context
    }

    // Step 2: Dispose of the V8 isolate
    if (isolate) {
        isolate->Dispose();  // Clean up the V8 isolate
        isolate = nullptr;    // Null out the reference to avoid dangling pointers
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
  }

  return 0; // Success
}

int NodeInstance::Tick(void* env)
{
  if (!env) {
    return -1; // Error: No environment
  }

  // Process all pending events without blocking
  while (uv_run(uv_default_loop(), UV_RUN_NOWAIT)) {
    // Loop until there are no more immediate events left to process
  }

  // Process any microtasks (e.g., resolved Promises)
  auto isolate = pImpl->isolatesMap[env];
  if (!isolate) {
    return -1;
  }

  isolate->PerformMicrotaskCheckpoint();

  return 0; // Success
}

int NodeInstance::ExecuteScript(void* env, const char* script)
{
  if (!env || !script) {
    return -1;
  }

  Isolate* isolate = pImpl->isolatesMap[env];
  if (!isolate) {
    return -1;
  }

  Isolate::Scope isolate_scope(isolate);
  HandleScope handle_scope(isolate);

  auto &contextPersistent = pImpl->contextsMap[env];
  Local<Context> context = contextPersistent.Get(isolate);
  Context::Scope context_scope(context);
  
  Local<String> source = String::NewFromUtf8(isolate, script).ToLocalChecked();
  Local<Script> compiled_script;

  if (!Script::Compile(context, source).ToLocal(&compiled_script)) {
    return -1; // Compilation error
  }

  compiled_script->Run(context); // Execute script
  
  return 0;
}

uint64_t NodeInstance::GetError(char* buffer, uint64_t bufferSize)
{
  // TODO
}
