#include "AsyncSaveStorage.h"
#include <thread>

struct AsyncSaveStorage::Impl
{
  struct UpsertTask
  {
    std::vector<MpChangeForm> changeForms;
    std::function<void()> callback;
  };

  std::shared_ptr<spdlog::logger> logger;

  struct
  {
    std::shared_ptr<IDatabase> dbImpl;
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
    std::mutex m;
  } share4;

  std::unique_ptr<std::thread> thr;
  std::atomic<bool> destroyed = false;
  uint32_t numFinishedUpserts = 0;
};

AsyncSaveStorage::AsyncSaveStorage(const std::shared_ptr<IDatabase>& dbImpl,
                                   std::shared_ptr<spdlog::logger> logger)
  : pImpl(new Impl, [](Impl* p) { delete p; })
{
  pImpl->logger = logger;
  pImpl->share.dbImpl = dbImpl;

  auto p = this->pImpl.get();
  pImpl->thr.reset(new std::thread([p] { SaverThreadMain(p); }));
}

AsyncSaveStorage::~AsyncSaveStorage()
{
  pImpl->destroyed = true;
  pImpl->thr->join();
}

void AsyncSaveStorage::SaverThreadMain(Impl* pImpl)
{
  while (!pImpl->destroyed) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    try {

      decltype(pImpl->share3.upsertTasks) tasks;
      {
        std::lock_guard l(pImpl->share3.m);
        tasks = std::move(pImpl->share3.upsertTasks);
        pImpl->share3.upsertTasks.clear();
      }

      std::vector<std::function<void()>> callbacksToFire;

      {
        std::lock_guard l(pImpl->share.m);
        auto was = clock();
        size_t numChangeForms = 0;
        for (auto& t : tasks) {
          numChangeForms += pImpl->share.dbImpl->Upsert(t.changeForms);
          callbacksToFire.push_back(t.callback);
        }
        if (numChangeForms > 0 && pImpl->logger)
          pImpl->logger->trace("Saved {} ChangeForms in {} ticks",
                               numChangeForms, clock() - was);
      }

      {
        std::lock_guard l(pImpl->share4.m);
        for (auto& cb : callbacksToFire)
          pImpl->share4.upsertCallbacksToFire.push_back(cb);
      }
    } catch (...) {
      std::lock_guard l(pImpl->share2.m);
      auto exceptionPtr = std::current_exception();
      pImpl->share2.exceptions.push_back(exceptionPtr);
    }
  }
}

void AsyncSaveStorage::IterateSync(const IterateSyncCallback& cb)
{
  std::lock_guard l(pImpl->share.m);
  pImpl->share.dbImpl->Iterate(cb);
}

void AsyncSaveStorage::Upsert(const std::vector<MpChangeForm>& changeForms,
                              const UpsertCallback& cb)
{
  std::lock_guard l(pImpl->share3.m);
  pImpl->share3.upsertTasks.push_back({ changeForms, cb });
}

uint32_t AsyncSaveStorage::GetNumFinishedUpserts() const
{
  return pImpl->numFinishedUpserts;
}

void AsyncSaveStorage::Tick()
{
  {
    std::lock_guard l(pImpl->share2.m);
    if (!pImpl->share2.exceptions.empty()) {
      auto exceptionPtr = std::move(pImpl->share2.exceptions.front());
      pImpl->share2.exceptions.pop_front();
      std::rethrow_exception(exceptionPtr);
    }
  }

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
}
