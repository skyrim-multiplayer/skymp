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
  res["dynamicFields"] = changeForm.dynamicFields;

  if (changeForm.lookDump.empty()) {
    res["lookDump"] = nullptr;
  } else {
    res["lookDump"] = nlohmann::json::parse(changeForm.lookDump);
  }

  if (changeForm.equipmentDump.empty()) {
    res["equipmentDump"] = nullptr;
  } else {
    res["equipmentDump"] = nlohmann::json::parse(changeForm.equipmentDump);
  }
  return res;
}

MpChangeForm JsonToChangeForm(simdjson::dom::element& element)
{
  static const JsonPointer recType("recType"), formDesc("formDesc"),
    baseDesc("baseDesc"), position("position"), angle("angle"),
    worldOrCell("worldOrCell"), inv("inv"), isHarvested("isHarvested"),
    isOpen("isOpen"), baseContainerAdded("baseContainerAdded"),
    nextRelootDatetime("nextRelootDatetime"), isDisabled("isDisabled"),
    profileId("profileId"), isRaceMenuOpen("isRaceMenuOpen"),
    lookDump("lookDump"), equipmentDump("equipmentDump"),
    dynamicFields("dynamicFields");

  MpChangeForm res;
  ReadEx(element, recType, &res.recType);

  const char* tmp;
  simdjson::dom::element jTmp;

  ReadEx(element, formDesc, &tmp);
  res.formDesc = FormDesc::FromString(tmp);

  ReadEx(element, baseDesc, &tmp);
  res.baseDesc = FormDesc::FromString(tmp);

  ReadEx(element, position, &jTmp);
  for (int i = 0; i < 3; ++i)
    ReadEx(jTmp, i, &res.position[i]);

  ReadEx(element, angle, &jTmp);
  for (int i = 0; i < 3; ++i)
    ReadEx(jTmp, i, &res.angle[i]);

  ReadEx(element, worldOrCell, &res.worldOrCell);

  ReadEx(element, inv, &jTmp);
  res.inv = Inventory::FromJson(jTmp);

  ReadEx(element, isHarvested, &res.isHarvested);
  ReadEx(element, isOpen, &res.isOpen);
  ReadEx(element, baseContainerAdded, &res.baseContainerAdded);
  ReadEx(element, nextRelootDatetime, &res.nextRelootDatetime);
  ReadEx(element, isDisabled, &res.isDisabled);
  ReadEx(element, profileId, &res.profileId);
  ReadEx(element, isRaceMenuOpen, &res.isRaceMenuOpen);

  ReadEx(element, lookDump, &jTmp);
  res.lookDump = simdjson::minify(jTmp);
  if (res.lookDump == "null")
    res.lookDump.clear();

  ReadEx(element, equipmentDump, &jTmp);
  res.equipmentDump = simdjson::minify(jTmp);
  if (res.equipmentDump == "null")
    res.equipmentDump.clear();

  try {
    simdjson::dom::element jDynamicFields;
    ReadEx(element, dynamicFields, &jDynamicFields);
    res.dynamicFields = nlohmann::json::parse(
      static_cast<std::string>(simdjson::minify(jDynamicFields)));
  } catch (JsonIndexException&) {
  } catch (...) {
    throw;
  }

  return res;
}
}

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

void MongoDatabase::Iterate(const IterateCallback& iterateCallback)
{
  auto emptyFilter = nlohmann::json::object();
  auto emptyFilterBson = bsoncxx::from_json(emptyFilter.dump());

  simdjson::dom::parser p;

  auto cursor = pImpl->changeFormsCollection->find(std::move(emptyFilterBson));
  for (auto& documentView : cursor) {
    auto document = p.parse(bsoncxx::to_json(documentView)).value();
    auto changeForm = JsonToChangeForm(document);
    iterateCallback(changeForm);
  }
}