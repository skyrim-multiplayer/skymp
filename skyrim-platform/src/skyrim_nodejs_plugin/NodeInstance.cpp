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

    // Store the environment for later use
    *outEnv = static_cast<void*>(env);
  }

  return 0; // Success
}

int NodeInstance::DestroyEnvironment(void* env)
{
  if (env != nullptr) {
    node::Environment* node_env = static_cast<node::Environment*>(env);

    // Close environment handles and tear down
    node_env->Dispose();

    Isolate* isolate = node_env->isolate();
    isolate->Dispose(); // Clean up V8 isolate

    auto create_params = pImpl->createParamsMap[env];

    if (create_params) {
      delete create_params->array_buffer_allocator;
    }

    pImpl->createParamsMap.erase(env);
  }

  return 0; // Success
}

int NodeInstance::Tick(void* env)
{
  if (env == nullptr)
    return -1; // Error: No environment

  node::Environment* node_env = static_cast<node::Environment*>(env);
  node::IsolateData* isolate_data = node_env->isolate_data();

  // Process all pending events without blocking
  while (uv_run(node_env->event_loop(), UV_RUN_NOWAIT)) {
    // Loop until there are no more immediate events left to process
  }

  // Process any microtasks (e.g., resolved Promises)
  node_env->isolate()->RunMicrotasks();

  return 0; // Success
}

int NodeInstance::ExecuteScript(void* env, const char* script)
{
  if (!env || !script) return -1;
  
  node::Environment* node_env = static_cast<node::Environment*>(env);
  Isolate* isolate = node_env->isolate();
  Isolate::Scope isolate_scope(isolate);
  HandleScope handle_scope(isolate);
  Local<Context> context = node_env->context();
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
