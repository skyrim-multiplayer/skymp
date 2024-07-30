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
  std::mutex iterateCallbackMutex;

  constexpr int kBatchSize = 1001;
  mongocxx::options::find findOptions;
  findOptions.batch_size(kBatchSize);

  auto totalDocuments = GetDocumentCount();

  constexpr int kNumParts = 100;

  int partSize = totalDocuments / kNumParts;

  std::atomic<int> totalDocumentsProcessed = 0;

  std::vector<std::shared_ptr<std::thread>> threads;

  for (int i = 0; i < kNumParts; i++) {
    auto skip = i * partSize;
    auto limit = (i == kNumParts - 1) ? totalDocuments - skip : partSize;

    auto f = [skip, limit, &totalDocumentsProcessed, &iterateCallbackMutex,
              &iterateCallback, findOptions, this] {
      simdjson::dom::parser p;

      mongocxx::v_noabi::pool::entry poolEntry = pImpl->pool->acquire();

      mongocxx::v_noabi::collection collection =
        poolEntry->database(pImpl->name).collection(pImpl->collectionName);

      mongocxx::options::find options = findOptions;
      options.skip(skip);
      options.limit(limit);

      auto cursor = collection.find({}, options);

      for (auto& documentView : cursor) {
        auto document = p.parse(bsoncxx::to_json(documentView)).value();
        auto changeForm = MpChangeForm::JsonToChangeForm(document);

        std::lock_guard<std::mutex> lock(iterateCallbackMutex);
        iterateCallback(changeForm);
        totalDocumentsProcessed++;
      }
    };

    threads.push_back(std::make_shared<std::thread>(f));
  }

  for (auto& thread : threads) {
    thread->join();
  }

  if (totalDocumentsProcessed.load() == totalDocuments) {
    spdlog::info("All documents processed: {}", totalDocuments);
  } else {
    spdlog::critical("Not all documents processed: {} / {}",
                     totalDocumentsProcessed.load(), totalDocuments);
    std::terminate();
  }
}

int MongoDatabase::GetDocumentCount()
{
  mongocxx::v_noabi::pool::entry poolEntry = pImpl->pool->acquire();
  mongocxx::v_noabi::collection collection =
    poolEntry->database(pImpl->name).collection(pImpl->collectionName);

  return collection.count_documents({});
}
