#include "MongoWithCacheDatabase.h"

#include "ArchiveDatabase.h"
#include "MongoDatabase.h"

struct MongoWithCacheDatabase::Impl
{
  std::shared_ptr<MongoDatabase> mongoDb;
  std::shared_ptr<ArchiveDatabase> archiveDb;
  std::shared_ptr<spdlog::logger> logger;
};

MongoWithCacheDatabase::MongoWithCacheDatabase(
  std::string uri_, std::string name_, std::shared_ptr<spdlog::logger> logger_)
{
  std::string archiveFilePath = name_ + "_cache.zip";

  pImpl = std::make_shared<Impl>();
  pImpl->mongoDb = std::make_shared<MongoDatabase>(uri_, name_);
  pImpl->archiveDb =
    std::make_shared<ArchiveDatabase>(archiveFilePath, logger_);
  pImpl->logger = logger_;
}

UpsertResult MongoWithCacheDatabase::Upsert(
  std::vector<std::optional<MpChangeForm>>&& changeForms)
{
  std::vector<std::optional<MpChangeForm>> changeFormsCopy = changeForms;
  std::vector<std::optional<MpChangeForm>> changeFormsCopy2 = changeFormsCopy;

  UpsertResult res = pImpl->mongoDb->Upsert(std::move(changeFormsCopy));

  // This is only for caching, so we don't really care if it fails
  try {
    pImpl->archiveDb->Upsert(std::move(changeFormsCopy2));

    if (res.changeFormsCollectionHash.has_value()) {
      pImpl->archiveDb->SetDbHash(*res.changeFormsCollectionHash);
    } else {
      pImpl->logger->warn("UpsertResult changeFormsCollectionHash is empty");
    }

    pImpl->archiveDb->WriteArchiveChecksumExpected(
      pImpl->archiveDb->GetArchiveChecksum());
  } catch (const std::exception& e) {
    pImpl->logger->warn("Failed to upsert to archive database: {}", e.what());
  }

  return res;
}

void MongoWithCacheDatabase::Iterate(const IterateCallback& iterateCallback)
{
  EnsureArchiveMatchesCrc32();
  EnsureArchiveMatchesMongoDbHash();

  if (pImpl->archiveDb->Exists()) {
    spdlog::info("Using archive database");
    pImpl->archiveDb->Iterate(iterateCallback);
  } else {
    spdlog::info("Using mongo database");
    pImpl->mongoDb->Iterate(iterateCallback);
  }
}

void MongoWithCacheDatabase::EnsureArchiveMatchesCrc32()
{
  auto archiveChecksum = pImpl->archiveDb->GetArchiveChecksum();

  pImpl->logger->info("Archive crc32 checksum: {}", archiveChecksum);

  auto archiveCheskumExpected =
    pImpl->archiveDb->ReadArchiveChecksumExpected();

  pImpl->logger->info("Archive crc32 checksum expected: {}",
                      archiveCheskumExpected.value_or(0));

  if (archiveChecksum != archiveCheskumExpected.value_or(0)) {
    pImpl->logger->info("Archive crc32 checksum mismatch, unlinking");
    pImpl->archiveDb->Unlink();
  } else {
    pImpl->logger->info("Archive crc32 checksum match");
  }
}

void MongoWithCacheDatabase::EnsureArchiveMatchesMongoDbHash()
{
  auto dbHash = pImpl->mongoDb->DbHash().changeFormsCollectionHash;
  auto archiveDbHash = pImpl->archiveDb->GetDbHash();

  if (dbHash != archiveDbHash) {
    pImpl->logger->info("Archive db hash mismatch, unlinking");
    pImpl->archiveDb->Unlink();
  } else {
    pImpl->logger->info("Archive db hash match");
  }
}
