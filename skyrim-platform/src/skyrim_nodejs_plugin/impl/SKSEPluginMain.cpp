#include <Windows.h>
#include <cstdint>

#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

#include "NodeInstance.h"

NodeInstance& GetNodeInstance()
{
  static NodeInstance g_nodeInstance;
  return g_nodeInstance;
}

extern "C" {

// Initialize the Node.js engine. This function should be called before any
// other NodeJS-related function.
// Returns 0 on success, or a non-zero error code on failure.
__declspec(dllexport) int SkyrimNodeJS_Init(int argc, char** argv)
{
  return GetNodeInstance().Init(argc, argv);
}

// Create a new Node.js runtime environment (equivalent to a new V8 Isolate).
// Returns a handle to the environment that can be used in subsequent calls.
// Returns 0 on success, or a non-zero error code on failure.
__declspec(dllexport) int SkyrimNodeJS_CreateEnvironment(int argc, char** arg,
                                                         void** outEnv)
{
  return GetNodeInstance().CreateEnvironment(argc, arg, outEnv);
}

// Destroy the Node.js runtime environment and free resources.
// This should be called when the Node.js environment is no longer needed.
// Returns 0 on success, or a non-zero error code on failure.
__declspec(dllexport) int SkyrimNodeJS_DestroyEnvironment(void* env)
{
  return GetNodeInstance().DestroyEnvironment(env);
}

// Tick the Node.js event loop. This should be called regularly to allow
// Node.js to process async events.
// Returns 0 on success, or a non-zero error code on failure.
__declspec(dllexport) int SkyrimNodeJS_Tick(void* env)
{
  return GetNodeInstance().Tick(env);
}

// Execute a JavaScript script in the Node.js environment.
// Takes the environment handle and the JavaScript code as a string.
// Returns 0 on success, or a non-zero error code on failure.
__declspec(dllexport) int SkyrimNodeJS_ExecuteScript(void* env,
                                                     const char* script)
{
  return GetNodeInstance().ExecuteScript(env, script);
}

// Get the last error message from the Node.js environment.
// Returns the length of the error message.
__declspec(dllexport) uint64_t SkyrimNodeJS_GetError(char* buffer,
                                                     uint64_t bufferSize)
{
  return GetNodeInstance().GetError(buffer, bufferSize);
}

__declspec(dllexport) bool SkyrimNodeJS_OnSKSEPluginLoad(void* skse)
{
  return true;
}
}
