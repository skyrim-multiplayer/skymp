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

// Thread handle compatible with std::thread duck-typing (has join())
class NapiWorkerThread
{
public:
  void join()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return finished_.load(); });
  }

  void NotifyThreadFunctionBodyCompleted()
  {
    finished_.store(true);
    cv_.notify_all();
  }

private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic<bool> finished_{ false };
};

struct NapiWorkerThreadRegistry
{
  std::mutex mutex;
  uint64_t nextId = 1;
  std::map<uint64_t, std::function<void()>> bodies;

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

inline Napi::Value ExecuteWorkerBody(const Napi::CallbackInfo& info)
{
  auto bodyId =
    static_cast<uint64_t>(info[0].As<Napi::Number>().DoubleValue());
  auto body = NapiWorkerThreadRegistry::Instance().Take(bodyId);
  body();
  return info.Env().Undefined();
}

inline void InitNapiWorkerThread(Napi::Env env, Napi::Object exports)
{
  exports.Set("_executeWorkerBody",
              Napi::Function::New(env, ExecuteWorkerBody));
}

// Creates a factory function compatible with AsyncSaveStorage::ThreadFactory
inline std::function<std::unique_ptr<NapiWorkerThread>(std::function<void()>)>
CreateNapiWorkerThreadFactory(Napi::Env env, const std::string& addonPath)
{
  if (addonPath.empty()) {
    throw std::runtime_error("Failed to resolve addon (.node) file path");
  }

  return [env, addonPath](
           std::function<void()> body) -> std::unique_ptr<NapiWorkerThread> {
    auto thread = std::make_unique<NapiWorkerThread>();
    auto* threadPtr = thread.get();

    auto wrappedBody = [body = std::move(body), threadPtr]() {
      body();
      threadPtr->NotifyThreadFunctionBodyCompleted();
    };

    uint64_t bodyId =
      NapiWorkerThreadRegistry::Instance().Register(std::move(wrappedBody));

    // JSON-escape the addon path for safe embedding in JS string
    nlohmann::json pathJson = addonPath;
    std::string jsAddonPath = pathJson.dump(); // produces "escaped/path"

    std::string js = "(() => {"
                     "  const _require = global.require || "
                     "global.process.mainModule.constructor._load;"
                     "  const { Worker } = _require('worker_threads');"
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
