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

#include "NapiHelper.h"

class JsEngine
{
public:
  JsEngine()
    : pImpl(new Impl)
  {
    pImpl->argv.push_back(const_cast<char *>("node"));
    pImpl->argc = 1;
    auto argv = pImpl->argv.data();
    auto argc = pImpl->argc;

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
    pImpl->getError = (uint64_t (*)(char*, uint64_t))GetProcAddress(skyrimNodeJs, "SkyrimNodeJS_GetError");

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
    if (!pImpl->getError) {
      throw std::runtime_error("Unable to find SkyrimNodeJS_GetError: Error " + std::to_string(static_cast<int64_t>(GetLastError())));
    }

    static std::once_flag g_nodeInitFlag;

    std::call_once(g_nodeInitFlag, [&]() {
      if (pImpl->init(argc, argv) != 0) {
        size_t errorSize = pImpl->getError(NULL, 0);
        char* error = new char[errorSize];
        pImpl->getError(error, errorSize);
        std::string s = fmt::format("SkyrimNodeJS: Failed to initialize {}", error);
        delete[] error;
        throw std::runtime_error(s);
      }
    });

    // create environment
    if (pImpl->createEnvironment(argc, argv, &pImpl->env) != 0) {
      size_t errorSize = pImpl->getError(NULL, 0);
      char* error = new char[errorSize];
      pImpl->getError(error, errorSize);
      std::string s = fmt::format("SkyrimNodeJS: Failed to create environment {}", error);
      delete[] error;
      throw std::runtime_error(s);
    }
  }

  ~JsEngine() { 
    if (pImpl->env) {
      if (pImpl->destroyEnvironment(pImpl->env) != 0) {
        size_t errorSize = pImpl->getError(NULL, 0);
        char* error = new char[errorSize];
        pImpl->getError(error, errorSize);
        std::string s = fmt::format("SkyrimNodeJS: Failed to destroy environment {}", error);
        delete[] error;
        std::cerr << s << std::endl;
      }
    }
    delete pImpl; 
  }

  Napi::Env Env()
  {
    napi_env env = reinterpret_cast<napi_env>(pImpl->env);
    return Napi::Env(env);
  }

  Napi::Value RunScript(const std::string& src, const std::string&)
  {
    return NapiHelper::RunScript(Env(), src.data());
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
    uint64_t (*getError)(char*, uint64_t) = nullptr;

    void *env = nullptr;

    std::vector<char *> argv;
    int argc = 0;
  };

  Impl* const pImpl;
};
