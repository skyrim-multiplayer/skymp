#pragma once

namespace InventoryApi {

JsValue GetExtraContainerChanges(const JsFunctionArguments& args);
JsValue GetContainer(const JsFunctionArguments& args);
JsValue SetInventory(const JsFunctionArguments& args);

void Register(JsValue& exports);
}
