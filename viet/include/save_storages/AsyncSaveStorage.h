#pragma once
#include "ISaveStorage.h"
#include "database_drivers/IDatabase.h"
#include <atomic>
#include <chrono>
#include <list>
#include <mutex>
#include <spdlog/logger.h>
#include <stdexcept>
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

  class UpsertFailedException : public std::runtime_error
  {
  public:
    UpsertFailedException(std::vector<std::optional<T>>&& affectedForms_,
                          std::string what)
      : runtime_error(what)
      , affectedForms(affectedForms_)
    {
    }

    const std::vector<std::optional<T>>& GetAffectedForms() const noexcept
    {
      return affectedForms;
    }

  private:
    const std::vector<std::optional<T>> affectedForms;
  };

  class IterateFailedException : public std::runtime_error
  {
  public:
    IterateFailedException(std::optional<std::vector<FormDescType>>&& filter_,
                           std::string what)
      : runtime_error(what)
      , filter(std::move(filter_))
    {
    }

    const std::optional<std::vector<FormDescType>>& GetFilter() const noexcept
    {
      return filter;
    }

  private:
    const std::optional<std::vector<FormDescType>> filter;
  };

  // logger must support multithreaded writing
  AsyncSaveStorage(const std::shared_ptr<IDatabase<T, FormDescType>>& dbImpl,
                   std::shared_ptr<spdlog::logger> logger = nullptr,
                   std::string name = "",
                   std::optional<uint32_t> sleepTimeMs = std::nullopt)
    : pImpl(std::make_shared<Impl>())
  {
    pImpl->name = std::move(name);
    pImpl->logger = logger;
    pImpl->share.dbImpl = dbImpl;

    if (sleepTimeMs.has_value()) {
      pImpl->sleepTimeMs = *sleepTimeMs;
    }

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
    return pImpl->tickStore.numFinishedUpserts;
  }

  uint32_t GetNumFinishedIterates() const override
  {
    return pImpl->tickStore.numFinishedIterates;
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

    TickUpsertCallbacks();
    TickIterateCallbacks();
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
  enum class CallbackGarbageMark
  {
    None,
    CanBeGarbageCollected,
  };

  struct TickCallbacksParams
  {
    struct
    {
      std::vector<std::pair<CallbackGarbageMark, std::function<void()>>>*
        preparedCallbacksToFire = nullptr;
      uint32_t* counter = nullptr;
    } tickStore;

    struct
    {
      std::vector<std::pair<CallbackGarbageMark, std::function<void()>>>*
        callbacksToFire = nullptr;
      std::mutex* m = nullptr;
    } share;
  };

  void TickUpsertCallbacks()
  {
    TickCallbacksParams params;
    params.tickStore.counter = &pImpl->tickStore.numFinishedUpserts;
    params.tickStore.preparedCallbacksToFire =
      &pImpl->tickStore.preparedUpsertCallbacksToFire;
    params.share.callbacksToFire = &pImpl->share4.upsertCallbacksToFire;
    params.share.m = &pImpl->share4.m;
    TickCallbacks(params);
  }

  void TickIterateCallbacks()
  {
    TickCallbacksParams params;
    params.tickStore.counter = &pImpl->tickStore.numFinishedIterates;
    params.tickStore.preparedCallbacksToFire =
      &pImpl->tickStore.preparedIterateCallbacksToFire;
    params.share.callbacksToFire = &pImpl->share5.iterateCallbacksToFire;
    params.share.m = &pImpl->share5.m;
    TickCallbacks(params);
  }

  void TickCallbacks(TickCallbacksParams& params)
  {
    std::erase_if(
      *params.tickStore.preparedCallbacksToFire, [](const auto& pair) {
        return pair.first == CallbackGarbageMark::CanBeGarbageCollected;
      });

    {
      std::lock_guard l(*params.share.m);

      // Reserve space to avoid mid-move allocations
      params.tickStore.preparedCallbacksToFire->reserve(
        params.tickStore.preparedCallbacksToFire->size() +
        params.share.callbacksToFire->size());

      // Move directly into the store
      std::move(params.share.callbacksToFire->begin(),
                params.share.callbacksToFire->end(),
                std::back_inserter(*params.tickStore.preparedCallbacksToFire));

      params.share.callbacksToFire->clear();
    }

    for (auto& [garbageState, cb] :
         *params.tickStore.preparedCallbacksToFire) {
      if (garbageState == CallbackGarbageMark::CanBeGarbageCollected) {
        continue;
      }
      garbageState = CallbackGarbageMark::CanBeGarbageCollected;
      cb();
      (*params.tickStore.counter)++;
    }
  }

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
      std::vector<std::pair<CallbackGarbageMark, std::function<void()>>>
        upsertCallbacksToFire;
      std::list<std::vector<std::optional<T>>> recycledChangeFormsBuffers;
      std::mutex m;
    } share4;

    struct
    {
      std::vector<std::pair<CallbackGarbageMark, std::function<void()>>>
        iterateCallbacksToFire;
      std::mutex m;
    } share5;

    struct
    {
      std::vector<IterateTask> iterateTasks;
      std::mutex m;
    } share6;

    struct
    {
      std::vector<std::pair<CallbackGarbageMark, std::function<void()>>>
        preparedUpsertCallbacksToFire;
      std::vector<std::pair<CallbackGarbageMark, std::function<void()>>>
        preparedIterateCallbacksToFire;

      uint32_t numFinishedUpserts = 0;
      uint32_t numFinishedIterates = 0;
    } tickStore;

    std::unique_ptr<std::thread> thr;
    std::atomic<bool> destroyed = false;
    uint32_t sleepTimeMs = 100;
  };

  std::shared_ptr<Impl> pImpl;

  static void ProcessUpserts(Impl* pImpl)
  {
    decltype(pImpl->share3.upsertTasks) tasks;
    {
      std::lock_guard l(pImpl->share3.m);
      tasks = std::move(pImpl->share3.upsertTasks);
      pImpl->share3.upsertTasks.clear();
    }

    struct TaskArtifact
    {
      std::function<void()> optionalCallbackToFire;
      std::exception_ptr optionalExceptionToFire;
      std::vector<std::optional<T>> optionalRecycledChangeFormsBuffer;
    };

    std::vector<TaskArtifact> taskArtifacts(tasks.size());

    {
      std::lock_guard l(pImpl->share.m);
      auto start = std::chrono::high_resolution_clock::now();
      size_t numChangeForms = 0;

      for (size_t i = 0; i < tasks.size(); ++i) {
        auto& t = tasks[i];
        auto& a = taskArtifacts[i];
        try {
          numChangeForms +=
            pImpl->share.dbImpl->Upsert(std::move(t.changeForms));
          t.changeForms.clear();
          a.optionalCallbackToFire = std::move(t.callback);

          std::vector<std::optional<T>> tmp;
          if (pImpl->share.dbImpl->GetRecycledChangeFormsBuffer(tmp)) {
            a.optionalRecycledChangeFormsBuffer = std::move(tmp);
          }
        } catch (...) {
          a.optionalExceptionToFire = std::current_exception();
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
      for (auto& taskArtifact : taskArtifacts) {
        if (taskArtifact.optionalCallbackToFire) {
          pImpl->share4.upsertCallbacksToFire.push_back(
            { CallbackGarbageMark::None,
              std::move(taskArtifact.optionalCallbackToFire) });
        }
        if (taskArtifact.optionalRecycledChangeFormsBuffer.size()) {
          pImpl->share4.recycledChangeFormsBuffers.push_back(
            std::move(taskArtifact.optionalRecycledChangeFormsBuffer));
        }
      }
    }

    {
      std::lock_guard l(pImpl->share2.m);
      for (auto& taskArtifact : taskArtifacts) {
        if (taskArtifact.optionalExceptionToFire) {
          pImpl->share2.exceptions.push_back(
            std::move(taskArtifact.optionalExceptionToFire));
        }
      }
    }
  }

  static void ProcessIterates(Impl* pImpl)
  {
    decltype(pImpl->share6.iterateTasks) tasks;
    {
      std::lock_guard l(pImpl->share6.m);
      tasks = std::move(pImpl->share6.iterateTasks);
      pImpl->share6.iterateTasks.clear();
    }

    std::vector<std::function<void()>> callbacksToFire;
    std::vector<std::exception_ptr> exceptionsToFire;

    const size_t tasksCount = tasks.size();
    callbacksToFire.reserve(tasksCount);
    exceptionsToFire.reserve(tasksCount);

    {
      std::lock_guard l(pImpl->share.m);
      for (auto& t : tasks) {
        try {
          std::vector<T> buffer;
          pImpl->share.dbImpl->Iterate(
            [&](const T& changeForm) { buffer.push_back(changeForm); },
            t.filter);
          std::function<void()> callback =
            [cb = t.callback, buf = std::move(buffer)]() { cb(buf); };
          callbacksToFire.push_back(callback);
        } catch (...) {
          exceptionsToFire.push_back(std::current_exception());
        }
      }
    }

    if (!callbacksToFire.empty()) {
      std::lock_guard l2(pImpl->share5.m);
      for (auto& callback : callbacksToFire) {
        pImpl->share5.iterateCallbacksToFire.push_back(
          { CallbackGarbageMark::None, callback });
      }
    }

    if (!exceptionsToFire.empty()) {
      std::lock_guard l(pImpl->share2.m);
      for (auto& exceptionPtr : exceptionsToFire) {
        pImpl->share2.exceptions.push_back(exceptionPtr);
      }
    }
  }

  static void SaverThreadMain(Impl* pImpl)
  {
    while (!pImpl->destroyed) {
      std::this_thread::sleep_for(
        std::chrono::milliseconds(pImpl->sleepTimeMs));
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
