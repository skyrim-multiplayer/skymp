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
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

#include <sw/redis++/redis++.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "ScopedTask.h"

namespace {
template <class Callback>
void RedisIterateKeysByPattern(const std::string& pattern,
                               sw::redis::Redis& redis,
                               const Callback& callback)
{
  int64_t cursor = 0;
  std::vector<std::string> keys;

  do {
    cursor = redis.scan(cursor, pattern, 1000, std::back_inserter(keys));

    if (!keys.empty()) {
      callback(keys.begin(), keys.end());
      keys.clear();
    }

  } while (cursor != 0);
}
}

struct MongoDatabase::Impl
{
  const std::string uri;
  const std::string name;
  const std::string redisVersionKey;

  const char* const collectionName = "changeForms";
  const char* const versionCollectionName = "version";

  std::shared_ptr<mongocxx::client> client;
  std::shared_ptr<mongocxx::database> db;
  std::shared_ptr<mongocxx::collection> changeFormsCollection,
    versionCollection;

  std::shared_ptr<sw::redis::Redis> redis;
};

MongoDatabase::MongoDatabase(std::string uri_, std::string name_,
                             std::optional<std::string> redisUri_)
{
  static mongocxx::instance g_instance;

  pImpl.reset(new Impl{
    uri_,
    name_,
  });

  pImpl->client.reset(new mongocxx::client(mongocxx::uri(pImpl->uri.data())));
  pImpl->db.reset(new mongocxx::database((*pImpl->client)[pImpl->name]));
  pImpl->changeFormsCollection.reset(
    new mongocxx::collection((*pImpl->db)[pImpl->collectionName]));
  pImpl->versionCollection.reset(
    new mongocxx::collection((*pImpl->db)[pImpl->versionCollectionName]));

  if (redisUri_) {
    spdlog::info("MongoDatabase::MongoDatabase - Redis URI: {}",
                 redisUri_.value());
    pImpl->redis.reset(new sw::redis::Redis(redisUri_.value()));
  } else {
    spdlog::info("MongoDatabase::MongoDatabase - No Redis URI");
  }
}

size_t MongoDatabase::Upsert(
  std::vector<std::optional<MpChangeForm>>&& changeForms)
{
  auto changeFormsVersion = GetCurrentTimestampIso8601();

  RedisMsetChangeForms(changeForms, changeFormsVersion);

  return MongoUpsertTransaction(std::move(changeForms), changeFormsVersion,
                                MongoUpsertTransactionMode::kAppendVersion);
}

void MongoDatabase::Iterate(const IterateCallback& iterateCallback)
{
  bool versionsMatch = false;

  if (pImpl->redis) {
    auto emptyFilter = nlohmann::json::object();
    auto emptyFilterBson = bsoncxx::from_json(emptyFilter.dump());
    auto versionCursor =
      pImpl->versionCollection->find(std::move(emptyFilterBson));
    for (auto& documentView : versionCursor) {
      auto document = bsoncxx::to_json(documentView);
      auto j = nlohmann::json::parse(document);

      if (!j["version"].is_array()) {
        throw std::runtime_error("MongoDatabase::Iterate - version is not an "
                                 "array: " +
                                 document);
      }

      std::vector<std::string> versionHistoryFromMongo;

      for (auto& version : j["version"]) {
        if (!version.is_string()) {
          throw std::runtime_error(
            "MongoDatabase::Iterate - version element is not a string: " +
            document);
        }
        versionHistoryFromMongo.push_back(version.get<std::string>());
      }

      std::vector<std::string> versionHistoryFromRedis;

      try {
        pImpl->redis->lrange(pImpl->redisVersionKey, 0, -1,
                             std::back_inserter(versionHistoryFromRedis));
      } catch (std::exception& e) {
        spdlog::error("MongoDatabase::Iterate - Redis error getting "
                      "changeforms version: {}, disabling redis",
                      e.what());
        pImpl->redis = nullptr;
      }

      // Version histories must be equal and non-empty
      // Empty doesn't make sense because it means that there were no Redis
      // upserts yet
      if (!versionHistoryFromRedis.empty() &&
          versionHistoryFromMongo == versionHistoryFromRedis) {
        versionsMatch = true;
        spdlog::info("MongoDatabase::Iterate - Versions match: {}",
                     fmt::join(versionHistoryFromMongo, ", "));
        break;
      }
    }
  }

  if (versionsMatch) {
    spdlog::info("MongoDatabase::Iterate - Loading from redis");
    RedisIterateKeysByPattern(
      MakeChangeFormRedisKeyWildcard(), *pImpl->redis,
      [&](auto begin, auto end) {
        std::vector<std::string> changeFormJsons;
        pImpl->redis->mget(begin, end, std::back_inserter(changeFormJsons));

        simdjson::dom::parser p;
        for (auto& changeFormJson : changeFormJsons) {
          auto document = p.parse(changeFormJson).value();
          auto changeForm = MpChangeForm::JsonToChangeForm(document);

          iterateCallback(changeForm);
        }
      });
  } else {
    spdlog::info("MongoDatabase::Iterate - Loading from mongo, feeding redis");
    auto emptyFilter = nlohmann::json::object();
    auto emptyFilterBson = bsoncxx::from_json(emptyFilter.dump());

    simdjson::dom::parser p;

    std::shared_ptr<sw::redis::Transaction> transaction;

    std::string initialRedisVersion = GetCurrentTimestampIso8601();

    if (pImpl->redis) {
      transaction.reset(
        new sw::redis::Transaction(pImpl->redis->transaction()));

      try {
        RedisIterateKeysByPattern(
          MakeChangeFormRedisKeyWildcard(), *pImpl->redis,
          [&](auto begin, auto end) { transaction->del(begin, end); });
      } catch (std::exception& e) {
        spdlog::error(
          "MongoDatabase::Iterate - Redis error del: {}, disabling redis",
          e.what());
        pImpl->redis = nullptr;
        transaction = nullptr;
      }
    }

    auto cursor =
      pImpl->changeFormsCollection->find(std::move(emptyFilterBson));
    for (auto& documentView : cursor) {
      std::string documentViewDump = bsoncxx::to_json(documentView);
      auto document = p.parse(documentViewDump).value();
      auto changeForm = MpChangeForm::JsonToChangeForm(document);

      iterateCallback(changeForm);

      if (transaction) {
        transaction->set(MakeChangeFormRedisKey(changeForm), documentViewDump);
      }
    }

    if (transaction) {
      transaction->del(pImpl->redisVersionKey);
      transaction->lpush(pImpl->redisVersionKey, initialRedisVersion);
      (void)transaction->exec();

      MongoUpsertTransaction({}, initialRedisVersion,
                             MongoUpsertTransactionMode::kReplaceVersion);
    }
  }
}

size_t MongoDatabase::MongoUpsertTransaction(
  std::vector<std::optional<MpChangeForm>>&& changeForms,
  const std::string& changeFormsVersion, MongoUpsertTransactionMode mode)
{
  try {
    std::shared_ptr<mongocxx::client_session> session(
      new mongocxx::client_session(pImpl->client->start_session()));

    session->start_transaction();

    std::vector<mongocxx::model::update_one> changeFormUpserts;
    changeFormUpserts.reserve(changeForms.size());
    for (auto& changeForm : changeForms) {
      if (changeForm == std::nullopt) {
        continue;
      }

      auto jChangeForm = MpChangeForm::ToJson(*changeForm);

      auto filter = nlohmann::json::object();
      filter["formDesc"] = changeForm->formDesc.ToString();

      auto upd = nlohmann::json::object();
      upd["$set"] = jChangeForm;

      changeFormUpserts.push_back(
        mongocxx::model::update_one(
          { std::move(bsoncxx::from_json(filter.dump())),
            std::move(bsoncxx::from_json(upd.dump())) })
          .upsert(true));
    }
    if (!changeFormUpserts.empty()) {
      pImpl->changeFormsCollection->bulk_write(changeFormUpserts);
    }

    auto emptyFilter = nlohmann::json::object();
    auto emptyFilterBson = bsoncxx::from_json(emptyFilter.dump());
    auto upd = nlohmann::json::object();

    if (mode == MongoUpsertTransactionMode::kReplaceVersion) {
      upd["$set"] = nlohmann::json::object();
      upd["$set"]["version"] = nlohmann::json::array();
      upd["$set"]["version"].push_back(changeFormsVersion);
    } else if (mode == MongoUpsertTransactionMode::kAppendVersion) {
      auto emptyFilter = nlohmann::json::object();
      auto emptyFilterBson = bsoncxx::from_json(emptyFilter.dump());
      std::optional<bsoncxx::document::value> value =
        pImpl->versionCollection->find_one(std::move(emptyFilterBson));
      upd["$set"] = nlohmann::json::object();
      upd["$set"]["version"] = nlohmann::json::array();

      if (value) {

        // TODO: handle bad version in db
        auto arr = nlohmann::json::parse(
          bsoncxx::to_json(value->view()["version"].get_array().value));

        for (auto& element : arr)
          upd["$set"]["version"].push_back(element);
      }

      upd["$set"]["version"].push_back(changeFormsVersion);
    } else {
      throw std::runtime_error("MongoDatabase::MongoUpsertTransaction - "
                               "Unknown mode: " +
                               std::to_string(static_cast<int>(mode)));
    }

    std::vector<mongocxx::model::update_one> versionUpserts;
    versionUpserts.push_back(mongocxx::model::update_one(
                               { std::move(emptyFilterBson),
                                 std::move(bsoncxx::from_json(upd.dump())) })
                               .upsert(true));
    pImpl->versionCollection->bulk_write(versionUpserts);

    session->commit_transaction();

    // TODO: Should take data from bulk.execute result instead?
    return changeForms.size();
  } catch (std::exception& e) {
    throw UpsertFailedException(std::move(changeForms), e.what());
  }
}

void MongoDatabase::RedisMsetChangeForms(
  const std::vector<std::optional<MpChangeForm>>& changeForms,
  const std::string& changeFormsVersion)
{
  if (!pImpl->redis) {
    return;
  }

  std::vector<std::pair<std::string, std::string>> redisData;
  redisData.reserve(changeForms.size());
  for (auto& changeForm : changeForms) {
    if (changeForm == std::nullopt) {
      continue;
    }
    redisData.emplace_back(MakeChangeFormRedisKey(*changeForm),
                           MpChangeForm::ToJson(*changeForm).dump());
  }

  redisData.emplace_back(pImpl->redisVersionKey, changeFormsVersion);

  try {
    pImpl->redis->mset(redisData.begin(), redisData.end());
  } catch (std::exception& e) {
    spdlog::error(
      "MongoDatabase::Upsert - Redis error mset: {}, disabling redis",
      e.what());
    pImpl->redis = nullptr;
  }
}

std::string MongoDatabase::GetCurrentTimestampIso8601()
{
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  std::tm now_tm = *std::gmtime(&now_c);
  char buffer[128];
  std::strftime(buffer, sizeof(buffer), "%FT%TZ", &now_tm);
  return std::string(buffer);
}

std::string MongoDatabase::MakeChangeFormRedisKey(
  const MpChangeForm& changeForm)
{
  return "skymp5-server:" + pImpl->name +
    ":changeForm:" + changeForm.formDesc.ToString();
}

std::string MongoDatabase::MakeChangeFormRedisKeyWildcard()
{
  return "skymp5-server:" + pImpl->name + ":changeForm:*";
}
