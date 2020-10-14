#pragma once
#include "ISaveStorage.h"
#include <functional>
#include <memory>
#include <vector>

class DbImpl
{
public:
  using IterateCallback = std::function<void(const MpChangeForm&)>;

  virtual ~DbImpl() = default;
  virtual size_t Upsert(const std::vector<MpChangeForm>& changeForms) = 0;
  virtual void Iterate(const IterateCallback& iterateCallback) = 0;
};

class SqliteSaveStorage : public ISaveStorage
{
public:
  SqliteSaveStorage(std::shared_ptr<DbImpl> dbImpl);
  ~SqliteSaveStorage();

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