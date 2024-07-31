#include "MongoDatabase.h"

#include "JsonUtils.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/document/element.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/document/view_or_value.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mutex>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <spdlog/spdlog.h>
#include <thread>

struct MongoDatabase::Impl
{
  const std::string uri;
  const std::string name;

  const char* const collectionName = "changeForms";

  std::shared_ptr<mongocxx::pool> pool;
};

MongoDatabase::MongoDatabase(std::string uri_, std::string name_)
{
  static mongocxx::instance g_instance;

  pImpl.reset(new Impl{ uri_, name_ });

  pImpl->pool.reset(new mongocxx::pool(mongocxx::uri(pImpl->uri.data())));
}

size_t MongoDatabase::Upsert(
  std::vector<std::optional<MpChangeForm>>&& changeForms)
{
  try {
    mongocxx::v_noabi::pool::entry poolEntry = pImpl->pool->acquire();

    mongocxx::v_noabi::collection collection =
      poolEntry->database(pImpl->name).collection(pImpl->collectionName);

    auto bulk = collection.create_bulk_write();
    for (auto& changeForm : changeForms) {
      if (changeForm == std::nullopt) {
        continue;
      }

      auto jChangeForm = MpChangeForm::ToJson(*changeForm);

      auto filter = nlohmann::json::object();
      filter["formDesc"] = changeForm->formDesc.ToString();

      auto upd = nlohmann::json::object();
      upd["$set"] = jChangeForm;

      bulk.append(mongocxx::model::update_one(
                    { std::move(bsoncxx::from_json(filter.dump())),
                      std::move(bsoncxx::from_json(upd.dump())) })
                    .upsert(true));
    }

    (void)bulk.execute();

    // TODO: Should take data from bulk.execute result instead?
    return changeForms.size();
  } catch (std::exception& e) {
    throw UpsertFailedException(std::move(changeForms), e.what());
  }
}

void MongoDatabase::Iterate(const IterateCallback& iterateCallback)
{
  constexpr int kBatchSize = 1001;
  mongocxx::options::find findOptions;
  findOptions.batch_size(kBatchSize);

  auto totalDocuments = GetDocumentCount();

  int numParts = std::min(totalDocuments, 100);

  int partSize = totalDocuments / numParts;

  std::atomic<int> totalDocumentsProcessed = 0;

  std::string hash;
  std::vector<std::shared_ptr<std::thread>> threads;
  std::vector<std::optional<std::string>> threadsErrors;
  std::vector<uint8_t> threadsSuccess;
  std::vector<std::string> threadsDocumentsJsonArray;

  threadsDocumentsJsonArray.resize(numParts);
  threadsErrors.resize(numParts);
  threadsSuccess.resize(numParts, 0);

  bool allFinished = false;
  int numAttempts = 0;

  while (!allFinished) {
    numAttempts++;

    int numThreadsToRun = 0;
    for (int i = 0; i < numParts; i++) {
      if (threadsSuccess[i] == 1) {
        continue;
      }
      numThreadsToRun++;
    }

    if (numattempts > 1) {
      spdlog::info("Spawning {} threads to load remaining ChangeForms",
                   numThreadsToRun);
    } else {
      spdlog::info("Spawning {} threads to load ChangeForms", numThreadsToRun);
    }

    for (int i = 0; i < numParts; i++) {
      if (threadsSuccess[i] == 1) {
        continue;
      }

      // space is to be replaced with ] in case of empty array
      threadsDocumentsJsonArray[i] = "[ ";

      auto skip = i * partSize;
      auto limit = (i == numParts - 1) ? totalDocuments - skip : partSize;

      auto f = [i, skip, limit, &totalDocumentsProcessed, &iterateCallback,
                &threadsSuccess, &threadsErrors, &threadsDocumentsJsonArray,
                findOptions, this] {
        try {
          simdjson::dom::parser p;

          mongocxx::v_noabi::pool::entry poolEntry = pImpl->pool->acquire();

          mongocxx::v_noabi::collection collection =
            poolEntry->database(pImpl->name).collection(pImpl->collectionName);

          mongocxx::options::find options = findOptions;
          options.skip(skip);
          options.limit(limit);

          auto cursor = collection.find({}, options);

          for (auto& documentView : cursor) {
            threadsDocumentsJsonArray[i] +=
              bsoncxx::to_json(documentView) + ",";
          }
          threadsErrors[i] = std::nullopt;
          threadsSuccess[i] = 1;
        } catch (std::exception& e) {
          threadsErrors[i] = e.what();
          threadsSuccess[i] = 0;
        }
      };

      threads.push_back(std::make_shared<std::thread>(f));
    }

    for (auto& thread : threads) {
      thread->join();
    }
    threads.clear();

    auto errorOrNull = GetCombinedErrorOrNull(threadsErrors);

    if (errorOrNull == std::nullopt) {
      spdlog::info("All documents fetched from the database. Num attempts: {}",
                   numAttempts);
      allFinished = true;
    } else {
      spdlog::warn("Error: {}", *errorOrNull);
      spdlog::info("Retrying failed threads. Num attempts: {}", numAttempts);
    }
  }

  for (int i = 0; i < numParts; i++) {
    threadsDocumentsJsonArray[i].back() = ']';

    auto documentsJsonArray = threadsDocumentsJsonArray[i];

    simdjson::dom::parser p;
    auto allDocs = p.parse(documentsJsonArray).value();

    auto documentAsArray = allDocs.get_array();

    for (auto document : documentAsArray) {
      auto changeForm = MpChangeForm::JsonToChangeForm(document);

      iterateCallback(changeForm);

      totalDocumentsProcessed++;
      hash = Sha256(hash + changeForm.formDesc.ToString());
    }
  }

  // If it's the same iech time, it means that the order of the changeforms
  // load is the same. Which is good for testing potential startup bugs.
  spdlog::info("Hash: {}", hash);

  if (totalDocumentsProcessed.load() == totalDocuments) {
    spdlog::info("All documents processed: {}", totalDocuments);
  } else {
    throw std::runtime_error(
      fmt::format("Not all documents processed: {} / {}",
                  totalDocumentsProcessed.load(), totalDocuments));
  }
}

int MongoDatabase::GetDocumentCount()
{
  mongocxx::v_noabi::pool::entry poolEntry = pImpl->pool->acquire();
  mongocxx::v_noabi::collection collection =
    poolEntry->database(pImpl->name).collection(pImpl->collectionName);

  return collection.count_documents({});
}

std::optional<std::string> MongoDatabase::GetCombinedErrorOrNull(
  const std::vector<std::optional<std::string>>& errorList)
{
  const int kMaxDisplayErrors = 5;

  std::vector<std::string> errorListNonNull;
  errorListNonNull.reserve(errorList.size());
  for (auto& error : errorList) {
    if (error != std::nullopt) {
      errorListNonNull.push_back(*error);
    }
  }

  if (!errorListNonNull.empty()) {
    std::string errorMessage;
    int displayCount =
      std::min(kMaxDisplayErrors, static_cast<int>(errorListNonNull.size()));

    for (int i = 0; i < displayCount; ++i) {
      errorMessage += fmt::format("Error #{}: {}\n", i, errorListNonNull[i]);
    }

    if (errorListNonNull.size() > kMaxDisplayErrors) {
      errorMessage += fmt::format("... ({} errors remaining)\n",
                                  errorListNonNull.size() - kMaxDisplayErrors);
    }

    return errorMessage;
  }

  return std::nullopt;
}

std::string MongoDatabase::BytesToHexString(const uint8_t* bytes,
                                            size_t length)
{
  static constexpr auto kHexDigits = "0123456789abcdef";

  std::string hexStr(length * 2, ' ');
  for (size_t i = 0; i < length; ++i) {
    hexStr[2 * i] = kHexDigits[(bytes[i] >> 4) & 0xF];
    hexStr[2 * i + 1] = kHexDigits[bytes[i] & 0xF];
  }

  return hexStr;
}

std::string MongoDatabase::Sha256(const std::string& str)
{
  uint8_t hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, str.c_str(), str.size());
  SHA256_Final(hash, &sha256);

  return BytesToHexString(hash, SHA256_DIGEST_LENGTH);
}
