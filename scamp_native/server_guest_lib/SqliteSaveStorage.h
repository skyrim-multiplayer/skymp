#pragma once
#include "ISaveStorage.h"
#include <memory>

class SqliteSaveStorage : public ISaveStorage
{
public:
  SqliteSaveStorage(const char* filename);
  ~SqliteSaveStorage();

  void IterateSync(const IterateSyncCallback& cb) override;
  void Upsert(const std::vector<MpChangeForm>& changeForms,
              const UpsertCallback& cb) override;
  void Tick() override;

private:
  struct Impl;
  std::unique_ptr<Impl, void (*)(Impl*)> pImpl;

  static void SaverThreadMain(Impl*);
};