#pragma once
#include "ISaveStorage.h"
#include "database_drivers/IDatabase.h"
#include <atomic>
#include <chrono>
#include <list>
#include <mutex>
#include <spdlog/logger.h>
#include <thread>

namespace Viet {

template <typename T, typename FormDescType>
class AsyncSaveStorage : public ISaveStorage<T, FormDescType>
{
public:
  using IterateSyncCallback =
    typename ISaveStorage<T, FormDescType>::IterateSyncCallback;
  using UpsertCallback =
    typename ISaveStorage<T, FormDescType>::UpsertCallback;
  using IterateCallback =
    typename ISaveStorage<T, FormDescType>::IterateCallback;

  // logger must support multithreaded writing
  AsyncSaveStorage(const std::shared_ptr<IDatabase<T, FormDescType>>& dbImpl,
                   std::shared_ptr<spdlog::logger> logger = nullptr,
                   std::string name = "")
    : pImpl(std::make_shared<Impl>())
  {
    pImpl->name = std::move(name);
    pImpl->logger = logger;
    pImpl->share.dbImpl = dbImpl;

    auto p = this->pImpl.get();
    pImpl->thr = std::make_unique<std::thread>([p] { SaverThreadMain(p); });
  }

  ~AsyncSaveStorage()
  {
    pImpl->destroyed = true;
    pImpl->thr->join();
  }

  void IterateSync(const IterateSyncCallback& cb) override
  {
    std::lock_guard l(pImpl->share.m);
    pImpl->share.dbImpl->Iterate(cb, std::nullopt);
  }

  void Upsert(std::vector<std::optional<T>>&& changeForms,
              const UpsertCallback& cb) override
  {
    std::lock_guard l(pImpl->share3.m);
    pImpl->share3.upsertTasks.push_back({ std::move(changeForms), cb });
  }

  void Iterate(const IterateCallback& cb,
               const std::optional<std::vector<FormDescType>>& filter) override
  {
    std::lock_guard l(pImpl->share6.m);
    pImpl->share6.iterateTasks.push_back({ filter, cb });
  }

  uint32_t GetNumFinishedUpserts() const override
  {
    return pImpl->numFinishedUpserts;
  }

  void Tick() override
  {
    {
      std::lock_guard l(pImpl->share2.m);
      if (!pImpl->share2.exceptions.empty()) {
        auto exceptionPtr = std::move(pImpl->share2.exceptions.front());
        pImpl->share2.exceptions.pop_front();
        std::rethrow_exception(exceptionPtr);
      }
    }

    // TODO: consider protecting against throwing callbacks
    decltype(pImpl->share4.upsertCallbacksToFire) upsertCallbacksToFire;
    {
      std::lock_guard l(pImpl->share4.m);
      upsertCallbacksToFire = std::move(pImpl->share4.upsertCallbacksToFire);
      pImpl->share4.upsertCallbacksToFire.clear();
    }
    for (auto& cb : upsertCallbacksToFire) {
      pImpl->numFinishedUpserts++;
      cb();
    }

    // TODO: consider protecting against throwing callbacks
    decltype(pImpl->share5.iterateCallbacksToFire) iterateCallbacksToFire;
    {
      std::lock_guard l(pImpl->share5.m);
      iterateCallbacksToFire = std::move(pImpl->share5.iterateCallbacksToFire);
      pImpl->share5.iterateCallbacksToFire.clear();
    }
    for (auto& cb : iterateCallbacksToFire) {
      cb();
    }
  }

  bool GetRecycledChangeFormsBuffer(
    std::vector<std::optional<T>>& changeForms) override
  {
    std::lock_guard l(pImpl->share4.m);
    if (pImpl->share4.recycledChangeFormsBuffers.empty()) {
      return false;
    }

    changeForms = std::move(pImpl->share4.recycledChangeFormsBuffers.front());
    pImpl->share4.recycledChangeFormsBuffers.pop_front();
    return true;
  }

  const std::string& GetName() const override { return pImpl->name; }

private:
  struct Impl
  {
    struct UpsertTask
    {
      std::vector<std::optional<T>> changeForms;
      std::function<void()> callback;
    };

    struct IterateTask
    {
      std::optional<std::vector<FormDescType>> filter;
      IterateCallback callback;
    };

    std::shared_ptr<spdlog::logger> logger;
    std::string name;

    struct
    {
      std::shared_ptr<IDatabase<T, FormDescType>> dbImpl;
      std::mutex m;
    } share;

    struct
    {
      std::list<std::exception_ptr> exceptions;
      std::mutex m;
    } share2;

    struct
    {
      std::vector<UpsertTask> upsertTasks;
      std::mutex m;
    } share3;

    struct
    {
      std::vector<std::function<void()>> upsertCallbacksToFire;
      std::list<std::vector<std::optional<T>>> recycledChangeFormsBuffers;
      std::mutex m;
    } share4;

    struct
    {
      std::vector<std::function<void()>> iterateCallbacksToFire;
      std::mutex m;
    } share5;

    struct
    {
      std::vector<IterateTask> iterateTasks;
      std::mutex m;
    } share6;

    std::unique_ptr<std::thread> thr;
    std::atomic<bool> destroyed = false;
    uint32_t numFinishedUpserts = 0;
  };

  std::shared_ptr<Impl> pImpl;

  static void ProcessUpserts(Impl* pImpl)
  {
    try {
      decltype(pImpl->share3.upsertTasks) tasks;
      {
        std::lock_guard l(pImpl->share3.m);
        tasks = std::move(pImpl->share3.upsertTasks);
        pImpl->share3.upsertTasks.clear();
      }

      std::vector<std::function<void()>> callbacksToFire;
      std::vector<std::vector<std::optional<T>>> recycledChangeFormsBuffers;

      {
        std::lock_guard l(pImpl->share.m);
        auto start = std::chrono::high_resolution_clock::now();
        size_t numChangeForms = 0;
        for (auto& t : tasks) {
          numChangeForms +=
            pImpl->share.dbImpl->Upsert(std::move(t.changeForms));
          t.changeForms.clear();
          callbacksToFire.push_back(t.callback);

          std::vector<std::optional<T>> tmp;
          if (pImpl->share.dbImpl->GetRecycledChangeFormsBuffer(tmp)) {
            recycledChangeFormsBuffers.push_back(std::move(tmp));
          }
        }
        if (numChangeForms > 0 && pImpl->logger) {
          auto end = std::chrono::high_resolution_clock::now();
          std::chrono::duration<double, std::milli> elapsed = end - start;
          pImpl->logger->trace("Saved {} ChangeForms in {} ms", numChangeForms,
                               elapsed.count());
        }
      }

      {
        std::lock_guard l(pImpl->share4.m);
        for (auto& cb : callbacksToFire) {
          pImpl->share4.upsertCallbacksToFire.push_back(cb);
        }
        for (auto& buf : recycledChangeFormsBuffers) {
          pImpl->share4.recycledChangeFormsBuffers.push_back(std::move(buf));
        }
      }
    } catch (...) {
      std::lock_guard l(pImpl->share2.m);
      auto exceptionPtr = std::current_exception();
      pImpl->share2.exceptions.push_back(exceptionPtr);
    }
  }

  static void ProcessIterates(Impl* pImpl)
  {
    try {
      decltype(pImpl->share6.iterateTasks) tasks;
      {
        std::lock_guard l(pImpl->share6.m);
        tasks = std::move(pImpl->share6.iterateTasks);
        pImpl->share6.iterateTasks.clear();
      }

      std::vector<std::function<void()>> callbacksToFire;
      {
        std::lock_guard l(pImpl->share.m);
        for (auto& t : tasks) {
          std::vector<T> buffer;
          pImpl->share.dbImpl->Iterate(
            [&](const T& changeForm) { buffer.push_back(changeForm); },
            t.filter);
          std::function<void()> callback =
            [cb = t.callback, buf = std::move(buffer)]() { cb(buf); };
          callbacksToFire.push_back(callback);
        }
      }

      {
        std::lock_guard l2(pImpl->share5.m);
        for (auto& callback : callbacksToFire) {
          pImpl->share5.iterateCallbacksToFire.push_back(callback);
        }
      }

    } catch (...) {
      std::lock_guard l(pImpl->share2.m);
      auto exceptionPtr = std::current_exception();
      pImpl->share2.exceptions.push_back(exceptionPtr);
    }
  }

  static void SaverThreadMain(Impl* pImpl)
  {
    while (!pImpl->destroyed) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      ProcessUpserts(pImpl);
      ProcessIterates(pImpl);
    }
  }

  AsyncSaveStorage(const AsyncSaveStorage&) = delete;
  AsyncSaveStorage& operator=(const AsyncSaveStorage&) = delete;

  AsyncSaveStorage(AsyncSaveStorage&&) = delete;
  AsyncSaveStorage& operator=(AsyncSaveStorage&&) = delete;
};

}
