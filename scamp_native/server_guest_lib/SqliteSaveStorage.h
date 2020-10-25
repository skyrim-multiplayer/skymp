#pragma once
#include "ISaveStorage.h"
#include <functional>
#include <memory>
#include <spdlog/logger.h>
#include <string>
#include <vector>

class DbImpl
{
public:
  using IterateCallback = std::function<void(const MpChangeForm&)>;

  virtual ~DbImpl() = default;
  virtual size_t Upsert(const std::vector<MpChangeForm>& changeForms) = 0;
  virtual void Iterate(const IterateCallback& iterateCallback) = 0;
};

class AsyncSaveStorage : public ISaveStorage
{
public:
  // logger must support multithreaded writing
  AsyncSaveStorage(const std::shared_ptr<DbImpl>& dbImpl,
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

class SqliteSaveStorage : public AsyncSaveStorage
{
public:
  // logger must support multithreaded writing
  SqliteSaveStorage(std::string filename,
                    std::shared_ptr<spdlog::logger> logger = nullptr);

private:
  std::shared_ptr<DbImpl> CreateDbImpl(std::string filename);
};