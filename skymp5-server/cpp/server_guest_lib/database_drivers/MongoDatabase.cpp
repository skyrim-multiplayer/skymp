#include "MongoDatabase.h"

#include "JsonUtils.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/client_session.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <nlohmann/json.hpp>
#include <optional>

constexpr auto kChangeFormsCollectionName = "changeForms";

namespace {
struct DbHashResult
{
  std::string changeFormsHash;
};

std::optional<DbHashResult> ParseDbHashResult(
  bsoncxx::v_noabi::document::value& hashResultView)
{
  static const JsonPointer kCollectionsJsonPointer("collections");
  static const JsonPointer kChangeFormsCollectionNameJsonPointer(
    kChangeFormsCollectionName);

  simdjson::dom::parser p;

  try {
    auto hashResult = p.parse(bsoncxx::to_json(hashResultView)).value();

    simdjson::dom::element collections;
    ReadEx(hashResult, kCollectionsJsonPointer, &collections);

    const char* changeFormsHash = "";
    ReadEx(collections, kChangeFormsCollectionNameJsonPointer,
           &changeFormsHash);

    return DbHashResult{ changeFormsHash };

  } catch (JsonIndexException&) {
    return std::nullopt;
  }
}

}

struct MongoDatabase::Impl
{
  const std::string uri;
  const std::string name;

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
    new mongocxx::collection((*pImpl->db)[kChangeFormsCollectionName]));
}

UpsertResult MongoDatabase::Upsert(
  std::vector<std::optional<MpChangeForm>>&& changeForms)
{
  try {
    mongocxx::client_session session = pImpl->client->start_session();

    session.start_transaction();

    auto bulk = pImpl->changeFormsCollection->create_bulk_write();
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

    // dbHash command
    bsoncxx::document::value dbHashCommand =
      bsoncxx::builder::stream::document{}
      << "dbHash" << 1 << "collections" << bsoncxx::builder::stream::open_array
      << kChangeFormsCollectionName << bsoncxx::builder::stream::close_array
      << bsoncxx::builder::stream::finalize;

    auto hashResultView =
      pImpl->db->run_command(session, dbHashCommand.view());

    std::optional<DbHashResult> hashResult = ParseDbHashResult(hashResultView);

    session.commit_transaction();

    // TODO: Should take data from bulk.execute result instead?
    return { changeForms.size(),
             hashResult.has_value()
               ? std::make_optional(hashResult->changeFormsHash)
               : std::nullopt };
  } catch (std::exception& e) {
    throw UpsertFailedException(std::move(changeForms), e.what());
  }
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
