

#include "ScampServer.h"

#ifndef NAPI_CPP_EXCEPTIONS
#  error NAPI_CPP_EXCEPTIONS must be defined or throwing from JS code would crash!
#endif

// #include <windows.h>

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  // while(!IsDebuggerPresent()) {
  //   Sleep(1);
  // }
  return ScampServer::Init(env, exports);
}

NODE_API_MODULE(scamp, Init)
