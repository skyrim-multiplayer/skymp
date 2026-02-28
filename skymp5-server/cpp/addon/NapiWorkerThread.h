#pragma once
#include "NapiHelper.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <napi.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

// Thread handle compatible with std::thread duck-typing (has join()).
// Represents a Node.js Worker Thread that runs a C++ body function.
class NapiWorkerThread
{
public:
  void join()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return finished_.load(); });
  }

  // Called by the wrapped body when it completes
  void MarkFinished()
  {
    finished_.store(true);
    cv_.notify_all();
  }

private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic<bool> finished_{ false };
};

// Process-wide singleton registry for worker thread bodies.
// Static data in the .so is shared between the main thread and Worker threads
// (same process, same shared object loaded once by the OS).
struct NapiWorkerThreadRegistry
{
  std::mutex mutex;
  uint64_t nextId = 1;
  std::map<uint64_t, std::function<void()>> bodies;
  std::string addonPath;

  static NapiWorkerThreadRegistry& Instance()
  {
    static NapiWorkerThreadRegistry instance;
    return instance;
  }

  uint64_t Register(std::function<void()> body)
  {
    std::lock_guard<std::mutex> lock(mutex);
    uint64_t id = nextId++;
    bodies[id] = std::move(body);
    return id;
  }

  std::function<void()> Take(uint64_t id)
  {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = bodies.find(id);
    if (it == bodies.end()) {
      throw std::runtime_error("Worker body not found for id " +
                               std::to_string(id));
    }
    auto body = std::move(it->second);
    bodies.erase(it);
    return body;
  }
};

// N-API function called from Worker thread's JS to execute the C++ body.
// The Worker has its own V8 isolate — Napi::Env is available on the stack
// for any V8 API work (e.g. v8::ValueDeserializer in the future).
inline Napi::Value ExecuteWorkerBody(const Napi::CallbackInfo& info)
{
  auto bodyId =
    static_cast<uint64_t>(info[0].As<Napi::Number>().DoubleValue());
  auto body = NapiWorkerThreadRegistry::Instance().Take(bodyId);
  body();
  return info.Env().Undefined();
}

// Call during addon Init to:
// 1) Register the _executeWorkerBody export (so Workers can call it)
// 2) Capture the addon .node file path (so Workers can require() it)
inline void InitNapiWorkerThread(Napi::Env env, Napi::Object exports)
{
  exports.Set("_executeWorkerBody",
              Napi::Function::New(env, ExecuteWorkerBody));

  // Resolve addon path via JS — the addon is loaded as
  //   require(process.cwd() + "/scam_native.node")
  // We find the .node path by looking at require.cache or using
  // require.resolve with the known convention.
  auto result = NapiHelper::RunScript(
    env,
    "(() => {"
    "  const path = require('path');"
    "  const cache = require.cache || {};"
    "  const keys = Object.keys(cache);"
    "  for (let i = keys.length - 1; i >= 0; i--) {"
    "    if (keys[i].endsWith('.node')) return keys[i];"
    "  }"
    "  return path.resolve(process.cwd(), 'scam_native.node');"
    "})()");
  if (result.IsString()) {
    NapiWorkerThreadRegistry::Instance().addonPath =
      result.As<Napi::String>().Utf8Value();
  }
}

// Creates a factory function compatible with
// AsyncSaveStorage<T, FD, NapiWorkerThread>::ThreadFactory.
// Must be called within a valid N-API callback scope (env must be live).
// The returned factory must also be invoked within the same callback scope
// (it is — AsyncSaveStorage constructor calls it synchronously).
inline std::function<std::unique_ptr<NapiWorkerThread>(std::function<void()>)>
CreateNapiWorkerThreadFactory(Napi::Env env)
{
  const auto& addonPath = NapiWorkerThreadRegistry::Instance().addonPath;
  if (addonPath.empty()) {
    throw std::runtime_error(
      "Addon path not initialized. Call InitNapiWorkerThread first.");
  }

  return [env, addonPath](
           std::function<void()> body) -> std::unique_ptr<NapiWorkerThread> {
    auto thread = std::make_unique<NapiWorkerThread>();
    auto* threadPtr = thread.get();

    // Wrap body to mark thread as finished when it completes
    auto wrappedBody = [body = std::move(body), threadPtr]() {
      body();
      threadPtr->MarkFinished();
    };

    uint64_t bodyId =
      NapiWorkerThreadRegistry::Instance().Register(std::move(wrappedBody));

    // JSON-escape the addon path for safe embedding in JS string
    nlohmann::json pathJson = addonPath;
    std::string jsAddonPath = pathJson.dump(); // produces "escaped/path"

    std::string js = "(() => {"
                     "  const { Worker } = require('worker_threads');"
                     "  new Worker("
                     "    `const { workerData } = require('worker_threads');"
                     "     const addon = require(workerData.addonPath);"
                     "     addon._executeWorkerBody(workerData.bodyId);`,"
                     "    { eval: true, workerData: { bodyId: " +
      std::to_string(bodyId) + ", addonPath: " + jsAddonPath +
      " } }"
      "  );"
      "})()";

    NapiHelper::RunScript(env, js);

    return thread;
  };
}
