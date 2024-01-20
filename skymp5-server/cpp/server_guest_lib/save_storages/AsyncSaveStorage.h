#pragma once
#include "ISaveStorage.h"
#include "database_drivers/IDatabase.h"
#include <list>
#include <spdlog/logger.h>

class AsyncSaveStorage : public ISaveStorage
{
public:
  // logger must support multithreaded writing
  AsyncSaveStorage(const std::shared_ptr<IDatabase>& dbImpl,
                   std::shared_ptr<spdlog::logger> logger = nullptr);
  ~AsyncSaveStorage();

  void IterateSync(const IterateSyncCallback& cb) override;
  void Upsert(const std::vector<MpChangeForm>& changeForms,
              const UpsertCallback& cb) override;
  uint32_t GetNumFinishedUpserts() const override;
  void Tick() override;

private:
  struct Impl;
  std::unique_ptr<Impl, void (*)(Impl*)> pImpl;

  static void SaverThreadMain(Impl*);
};
