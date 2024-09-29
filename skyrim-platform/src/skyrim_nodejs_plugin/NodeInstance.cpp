#include <iostream>
#include <libplatform/libplatform.h>
#include <memory>
#include <node.h>
#include <string>
#include <uv.h>
#include <v8.h>
#include <vector>

using namespace node;
using namespace v8;

#include "NodeInstance.h"

// namespace {
// // Function to run a basic Node.js instance
// int RunNodeInstance(node::MultiIsolatePlatform* platform,
//                     const std::vector<std::string>& args,
//                     const std::vector<std::string>& exec_args)
// {
//   // Initialize V8 and create a new isolate.
//   Isolate::CreateParams create_params;
//   create_params.array_buffer_allocator =
//     v8::ArrayBuffer::Allocator::NewDefaultAllocator();
//   Isolate* isolate = Isolate::New(create_params);

//   {
//     // Isolate and context scope management
//     Isolate::Scope isolate_scope(isolate);
//     HandleScope handle_scope(isolate);

//     Local<Context> context = Context::New(isolate);
//     Context::Scope context_scope(context);

//     // Simple JavaScript code to run
//     const char* js_code = "console.log('Hello from embedded Node.js!');";

//     // Compile and run the JavaScript code
//     Local<String> source =
//       String::NewFromUtf8(isolate, js_code, NewStringType::kNormal)
//         .ToLocalChecked();
//     Local<Script> script = Script::Compile(context, source).ToLocalChecked();
//     script->Run(context).ToLocalChecked();
//   }

//   // Cleanup V8
//   isolate->Dispose();
//   delete create_params.array_buffer_allocator;

//   return 0; // Exit code
// }
// }

struct NodeInstance::Impl
{
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

int NodeInstance::CreateEnvironment(int argc, char** arg, void** outEnv)
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
  Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
    v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  Isolate* isolate = Isolate::New(create_params);

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

    delete create_params.array_buffer_allocator;
  }

  return 0; // Success
}

int NodeInstance::Tick(void* env)
{
  if (env == nullptr)
    return -1; // Error: No environment

  node::Environment* node_env = static_cast<node::Environment*>(env);
  node::IsolateData* isolate_data = node_env->isolate_data();

  // Run one iteration of the event loop; returns after one event
  uv_run(node_env->event_loop(), UV_RUN_ONCE);

  // Run pending microtasks like Promises
  node_env->isolate()->RunMicrotasks();

  return 0; // Success
}

int NodeInstance::ExecuteScript(void* env, const char* script)
{
  // TODO
}

uint64_t NodeInstance::GetError(char* buffer, uint64_t bufferSize)
{
  // TODO
}

// int NodeInstance::NodeMain(int argc, char** argv)
// {
//   argv = uv_setup_args(argc, argv);
//   std::vector<std::string> args(argv, argv + argc);
//   // Parse Node.js CLI options, and print any errors that have occurred while
//   // trying to parse them.
//   std::unique_ptr<node::InitializationResult> result =
//     node::InitializeOncePerProcess(
//       args,
//       { node::ProcessInitializationFlags::kNoInitializeV8,
//         node::ProcessInitializationFlags::kNoInitializeNodeV8Platform });

//   for (const std::string& error : result->errors()) {
//     fprintf(stderr, "%s: %s\n", args[0].c_str(), error.c_str());
//   }

//   if (result->early_return() != 0) {
//     return result->exit_code();
//   }

//   // Create a v8::Platform instance. `MultiIsolatePlatform::Create()` is a way
//   // to create a v8::Platform instance that Node.js can use when creating
//   // Worker threads. When no `MultiIsolatePlatform` instance is present,
//   // Worker threads are disabled.
//   std::unique_ptr<MultiIsolatePlatform> platform =
//     MultiIsolatePlatform::Create(4);
//   V8::InitializePlatform(platform.get());
//   V8::Initialize();

//   // See below for the contents of this function.
//   int ret =
//     RunNodeInstance(platform.get(), result->args(), result->exec_args());

//   V8::Dispose();
//   V8::DisposePlatform();

//   node::TearDownOncePerProcess();
//   return ret;
// }
