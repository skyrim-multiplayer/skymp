#include "MongoDatabase.h"

#include "JsonUtils.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/document/element.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/document/view_or_value.hpp>
#include <bsoncxx/json.hpp>
#include <fstream>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>

// namespace {
// void CloseServer()
// {
//   std::ifstream f("server-settings.json");
//   if (!f.good()) {
//     return spdlog::error("server-settings.json is missing");
//   }

//   std::stringstream buffer;
//   buffer << f.rdbuf();

//   nlohmann::json serverSettings;
//   try {
//     serverSettings = nlohmann::json::parse(buffer.str());
//   } catch (std::exception& e) {
//     return spdlog::error("error parsing settings: {}", e.what());
//   }

//   f.close();

//   std::ofstream f("server-settings.json");
//   f << serverSettings.dump();
//   // NONONO DANGEROUS
// }
// }

struct MongoDatabase::Impl
{
  const std::string uri;
  const std::string name;

  const char* const collectionName = "changeForms";

  std::shared_ptr<mongocxx::client> client;
  std::shared_ptr<mongocxx::database> db;
  std::shared_ptr<mongocxx::collection> changeFormsCollection;
};

MongoDatabase::MongoDatabase(std::string uri_, std::string name_)
{
  static mongocxx::instance g_instance;

  pImpl.reset(new Impl{ uri_, name_ });

  pImpl->client.reset(new mongocxx::client(mongocxx::uri(pImpl->uri.data())));
  pImpl->db.reset(new mongocxx::database((*pImpl->client)[pImpl->name]));
  pImpl->changeFormsCollection.reset(
    new mongocxx::collection((*pImpl->db)[pImpl->collectionName]));
}

size_t MongoDatabase::Upsert(const std::vector<MpChangeForm>& changeForms)
{
  auto bulk = pImpl->changeFormsCollection->create_bulk_write();
  for (auto& changeForm : changeForms) {
    auto jChangeForm = MpChangeForm::ToJson(changeForm);

    auto filter = nlohmann::json::object();
    filter["formDesc"] = changeForm.formDesc.ToString();

    auto upd = nlohmann::json::object();
    upd["$set"] = jChangeForm;

    bulk.append(mongocxx::model::update_one(
                  { std::move(bsoncxx::from_json(filter.dump())),
                    std::move(bsoncxx::from_json(upd.dump())) })
                  .upsert(true));
  }

  std::optional<mongocxx::v_noabi::result::bulk_write> bulkResult =
    bulk.execute();

  if (!bulkResult) {
    spdlog::critical("Upsert - empty bulk result");
    // CloseServer();
    std::terminate();
  }

  int insertedCount = bulkResult->inserted_count();
  int upsertedCount = bulkResult->upserted_count();
  int total = insertedCount + upsertedCount;

  if (changeForms.size() != total) {
    spdlog::critical("Upsert - insertedCount {}, upsertedCount {}, total = "
                     "{}, but bulk contained {}",
                     insertedCount, upsertedCount, total, changeForms.size());
    // CloseServer();
    std::terminate();
  }

  return changeForms.size(); // Should take data from mongo instead?
}

void MongoDatabase::Iterate(const IterateCallback& iterateCallback)
{
  auto emptyFilter = nlohmann::json::object();
  auto emptyFilterBson = bsoncxx::from_json(emptyFilter.dump());

  simdjson::dom::parser p;

  auto cursor = pImpl->changeFormsCollection->find(std::move(emptyFilterBson));
  for (auto& documentView : cursor) {
    auto document = p.parse(bsoncxx::to_json(documentView)).value();
    auto changeForm = MpChangeForm::JsonToChangeForm(document);
    iterateCallback(changeForm);
  }
}
