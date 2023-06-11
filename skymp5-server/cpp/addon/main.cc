

#include "ScampServer.h"

#ifndef NAPI_CPP_EXCEPTIONS
#  error NAPI_CPP_EXCEPTIONS must be defined or throwing from JS code would crash!
#endif

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  return ScampServer::Init(env, exports);
}

NODE_API_MODULE(scamp, Init)
