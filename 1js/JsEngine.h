#pragma once
#include "TaskQueue.h"
#include <cstdint>
#include <cstring>
#include <fmt/ranges.h>
#include <memory>
#include <napi.h>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <fmt/format.h>
#include <iostream>

#include <Windows.h>

#include "../skyrim-platform/src/platform_se/skyrim_platform/NapiHelper.h"

class JsExternalObjectBase
{
public:
  virtual ~JsExternalObjectBase() = default;
};

class JsEngine
{
public:
  JsEngine()
    : pImpl(new Impl)
  {
    auto skyrimNodeJs = GetModuleHandleA("skyrim_nodejs_dll.dll");
    if (!skyrimNodeJs) {
      throw std::runtime_error("Unable to GetModuleHandleA skyrim_nodejs_dll.dll: Error " +
                               std::to_string(static_cast<int64_t>(GetLastError())));
    }

    pImpl->init = (int (*)(int, char**))GetProcAddress(skyrimNodeJs, "SkyrimNodeJS_Init");
    pImpl->createEnvironment =
      (int (*)(int, char**, void**))GetProcAddress(skyrimNodeJs, "SkyrimNodeJS_CreateEnvironment");
    pImpl->destroyEnvironment = (int (*)(void*))GetProcAddress(skyrimNodeJs, "SkyrimNodeJS_DestroyEnvironment");
    pImpl->tick = (int (*)(void*))GetProcAddress(skyrimNodeJs, "SkyrimNodeJS_Tick");
    pImpl->executeScript = (int (*)(void*, const char*))GetProcAddress(skyrimNodeJs, "SkyrimNodeJS_ExecuteScript");

    if (!pImpl->init) {
      throw std::runtime_error("Unable to find SkyrimNodeJS_Init: Error " + std::to_string(static_cast<int64_t>(GetLastError())));
    }
    if (!pImpl->createEnvironment) {
      throw std::runtime_error("Unable to find SkyrimNodeJS_CreateEnvironment: Error " + std::to_string(static_cast<int64_t>(GetLastError())));
    }
    if (!pImpl->destroyEnvironment) {
      throw std::runtime_error("Unable to find SkyrimNodeJS_DestroyEnvironment: Error " + std::to_string(static_cast<int64_t>(GetLastError())));
    }
    if (!pImpl->tick) {
      throw std::runtime_error("Unable to find SkyrimNodeJS_Tick: Error " + std::to_string(static_cast<int64_t>(GetLastError())));
    }
    if (!pImpl->executeScript) {
      throw std::runtime_error("Unable to find SkyrimNodeJS_ExecuteScript: Error " + std::to_string(static_cast<int64_t>(GetLastError())));
    }

    static std::once_flag g_nodeInitFlag;

    std::call_once(g_nodeInitFlag, [&]() {
      if (pImpl->init(argc, argv) != 0) {
        size_t errorSize = getError(NULL, 0);
        char* error = new char[errorSize];
        getError(error, errorSize);
        std::string error = fmt::format("SkyrimNodeJS: Failed to initialize {}", error);
        delete[] error;
        throw std::runtime_error(error);
      }
    });

    // create environment
    if (pImpl->createEnvironment(argc, argv, &pImpl->env) != 0) {
      size_t errorSize = getError(NULL, 0);
      char* error = new char[errorSize];
      getError(error, errorSize);
      std::string error = fmt::format("SkyrimNodeJS: Failed to create environment {}", error);
      delete[] error;
      throw std::runtime_error(error);
    }
  }

  ~JsEngine() { 
    if (pImpl->env) {
      if (pImpl->destroyEnvironment(pImpl->env) != 0) {
        size_t errorSize = getError(NULL, 0);
        char* error = new char[errorSize];
        getError(error, errorSize);
        std::string e = fmt::format("SkyrimNodeJS: Failed to destroy environment {}", error);
        delete[] error;
        std::cerr << e << std::endl;
      }
    }
    delete pImpl; 
  }

  Napi::Env Env()
  {
    napi_env *env = reinterpret_cast<napi_env*>(pImpl->env);
    return Napi::Env(env);
  }

  Napi::Value RunScript(const std::string& src, const std::string&)
  {
    // auto script = Napi::String::New(Env(), src.c_str());
    // auto env = Env();
    // env.RunScript(script);

    return NapiHelper::RunScript(Env(), src.data());

    // if (pImpl->executeScript(pImpl->env, src.c_str()) != 0) {
    //   size_t errorSize = getError(NULL, 0);
    //   char* error = new char[errorSize];
    //   getError(error, errorSize);
    //   std::string error = fmt::format("SkyrimNodeJS: Failed to execute script {}", error);
    //   delete[] error;
    //   throw std::runtime_error(error);
    // }

    // return Napi::Env(pImpl->env).Undefined();
  }

  void ResetContext(Viet::TaskQueue<Napi::Env>&)
  {
    return;
  }

  size_t GetMemoryUsage() const
  {
    // TODO
    return 0;
  }

private:
  struct Impl
  {
    // methods of dll
    int (*init)(int, char**) = nullptr;
    int (*createEnvironment)(int, char**, void**) = nullptr;
    int (*destroyEnvironment)(void*) = nullptr;
    int (*tick)(void*) = nullptr;
    int (*executeScript)(void*, const char*) = nullptr;

    void *env = nullptr;
  };

  Impl* const pImpl;
};
