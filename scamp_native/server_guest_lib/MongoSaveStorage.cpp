#include "MongoSaveStorage.h"

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
#include <nlohmann/json.hpp>

namespace {
nlohmann::json ToJson(const MpChangeForm& changeForm)
{
  auto res = nlohmann::json::object();
  res["recType"] = static_cast<int>(changeForm.recType);
  res["formDesc"] = changeForm.formDesc.ToString();
  res["baseDesc"] = changeForm.baseDesc.ToString();
  res["position"] = { changeForm.position[0], changeForm.position[1],
                      changeForm.position[2] };
  res["angle"] = { changeForm.angle[0], changeForm.angle[1],
                   changeForm.angle[2] };
  res["worldOrCell"] = changeForm.worldOrCell;
  res["inv"] = changeForm.inv.ToJson();
  res["isHarvested"] = changeForm.isHarvested;
  res["isOpen"] = changeForm.isOpen;
  res["baseContainerAdded"] = changeForm.baseContainerAdded;
  res["nextRelootDatetime"] = changeForm.nextRelootDatetime;
  res["isDisabled"] = changeForm.isDisabled;
  res["profileId"] = changeForm.profileId;
  res["isRaceMenuOpen"] = changeForm.isRaceMenuOpen;
  res["lookDump"] = changeForm.lookDump;
  res["equipmentDump"] = changeForm.equipmentDump;
  return res;
}

MpChangeForm JsonToChangeForm(simdjson::dom::element& element)
{
  MpChangeForm res;
  ReadEx(element, "recType", &res.recType);

  const char* tmp;
  simdjson::dom::element jTmp;

  ReadEx(element, "formDesc", &tmp);
  res.formDesc = FormDesc::FromString(tmp);

  ReadEx(element, "baseDesc", &tmp);
  res.baseDesc = FormDesc::FromString(tmp);

  ReadEx(element, "position", &jTmp);
  for (int i = 0; i < 3; ++i)
    ReadEx(jTmp, i, &res.position[i]);

  ReadEx(element, "angle", &jTmp);
  for (int i = 0; i < 3; ++i)
    ReadEx(jTmp, i, &res.angle[i]);

  ReadEx(element, "worldOrCell", &res.worldOrCell);

  ReadEx(element, "inv", &jTmp);
  res.inv = Inventory::FromJson(jTmp);

  ReadEx(element, "isHarvested", &res.isHarvested);
  ReadEx(element, "isOpen", &res.isOpen);
  ReadEx(element, "baseContainerAdded", &res.baseContainerAdded);
  ReadEx(element, "nextRelootDatetime", &res.nextRelootDatetime);
  ReadEx(element, "isDisabled", &res.isDisabled);
  ReadEx(element, "profileId", &res.profileId);
  ReadEx(element, "isRaceMenuOpen", &res.isRaceMenuOpen);

  ReadEx(element, "lookDump", &tmp);
  res.lookDump = tmp;

  ReadEx(element, "equipmentDump", &tmp);
  res.equipmentDump = tmp;

  return res;
}

class MongoDbImpl : public IDatabase
{
public:
  MongoDbImpl(std::string uri_, std::string name_)
    : uri(uri_)
    , name(name_)
  {
    client.reset(new mongocxx::client(mongocxx::uri(uri.data())));
    db.reset(new mongocxx::database((*client)[name]));
    changeFormsCollection.reset(
      new mongocxx::collection((*db)[collectionName]));
  }

  size_t Upsert(const std::vector<MpChangeForm>& changeForms)
  {
    auto bulk = changeFormsCollection->create_bulk_write();
    for (auto& changeForm : changeForms) {
      auto jChangeForm = ToJson(changeForm);

      auto filter = nlohmann::json::object();
      filter["formDesc"] = changeForm.formDesc.ToString();

      auto upd = nlohmann::json::object();
      upd["$set"] = jChangeForm;

      bulk.append(mongocxx::model::update_one(
                    { std::move(bsoncxx::from_json(filter.dump())),
                      std::move(bsoncxx::from_json(upd.dump())) })
                    .upsert(true));
    }

    (void)bulk.execute();
    return changeForms.size(); // Should take data from mongo instead?
  }

  void Iterate(const IterateCallback& iterateCallback)
  {
    auto emptyFilter = nlohmann::json::object();
    auto emptyFilterBson = bsoncxx::from_json(emptyFilter.dump());

    simdjson::dom::parser p;

    auto cursor = changeFormsCollection->find(std::move(emptyFilterBson));
    for (auto& documentView : cursor) {
      auto document = p.parse(bsoncxx::to_json(documentView)).value();
      auto changeForm = JsonToChangeForm(document);
      iterateCallback(changeForm);
    }
  }

private:
  const std::string uri, name;
  const char* const collectionName = "changeForms";

  std::shared_ptr<mongocxx::client> client;
  std::shared_ptr<mongocxx::database> db;
  std::shared_ptr<mongocxx::collection> changeFormsCollection;
};
}

MongoSaveStorage::MongoSaveStorage(std::string uri, std::string name,
                                   std::shared_ptr<spdlog::logger> logger)
  : AsyncSaveStorage(CreateDbImpl(uri, name), logger)
{
}

std::shared_ptr<IDatabase> MongoSaveStorage::CreateDbImpl(std::string uri,
                                                          std::string name)
{
  static mongocxx::instance g_instance;

  return std::make_shared<MongoDbImpl>(uri, name);
}