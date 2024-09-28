#include <node.h>               // For Node.js related functions
#include <v8.h>                 // For V8 API
#include <libplatform/libplatform.h> // For platform-specific functionality
#include <iostream>            // For standard I/O operations
#include <memory>              // For smart pointers
#include <string>              // For std::string
#include <vector>              // For std::vector
#include <uv.h>

using namespace node;
using namespace v8;

//#include <node/api.h>          // For Node API definitions
//#include <node/common.h>       // For Common functions
//#include <node/environment.h>  // For Environment class
//#include <node/node.h>         // For Node related functionalities
//#include <node/module.h>       // For module related functions
//#include <node/v8.h>           // For V8 related functions

#include "NodeInstance.h"

struct NodeInstance::Impl
{

};

NodeInstance::NodeInstance()
{
    pImpl = std::make_shared<Impl>();
}

void NodeInstance::Load()
{

}

int NodeInstance::NodeMain(int argc, char** argv)
{
  argv = uv_setup_args(argc, argv);
  std::vector<std::string> args(argv, argv + argc);
  // Parse Node.js CLI options, and print any errors that have occurred while
  // trying to parse them.
  std::unique_ptr<node::InitializationResult> result =
      node::InitializeOncePerProcess(args, {
        node::ProcessInitializationFlags::kNoInitializeV8,
        node::ProcessInitializationFlags::kNoInitializeNodeV8Platform
      });

  for (const std::string& error : result->errors()) {
    fprintf(stderr, "%s: %s\n", args[0].c_str(), error.c_str());
  }

  if (result->early_return() != 0) {
    return result->exit_code();
  }

  // Create a v8::Platform instance. `MultiIsolatePlatform::Create()` is a way
  // to create a v8::Platform instance that Node.js can use when creating
  // Worker threads. When no `MultiIsolatePlatform` instance is present,
  // Worker threads are disabled.
  std::unique_ptr<MultiIsolatePlatform> platform =
      MultiIsolatePlatform::Create(4);
  V8::InitializePlatform(platform.get());
  V8::Initialize();

  // See below for the contents of this function.
  int ret = RunNodeInstance(
      platform.get(), result->args(), result->exec_args());

  V8::Dispose();
  V8::DisposePlatform();

  node::TearDownOncePerProcess();
  return ret;
}
