#include "PropertyBindingFactory.h"

#include "ActorNeighborsBinding.h"
#include "AngleBinding.h"
#include "AppearanceBinding.h"
#include "BaseDescBinding.h"
#include "CustomPropertyBinding.h"
#include "EquipmentBinding.h"
#include "InventoryBinding.h"
#include "IsDeadBinding.h"
#include "IsDisabledBinding.h"
#include "IsOnlineBinding.h"
#include "IsOpenBinding.h"
#include "LocationalDataBinding.h"
#include "NeighborsBinding.h"
#include "OnlinePlayersBinding.h"
#include "PercentagesBinding.h"
#include "PosBinding.h"
#include "SpawnPointBinding.h"
#include "TypeBinding.h"
#include "WorldOrCellDescBinding.h"

std::map<std::string, std::shared_ptr<PropertyBinding>> PropertyBindingFactory::CreateStandardPropertyBindings() {
    std::map<std::string, std::shared_ptr<PropertyBinding>> result;
    result["actorNeighbors"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<ActorNeighborsBinding>());
    result["angle"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<AngleBinding>());
    result["appearance"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<AppearanceBinding>());
    result["baseDesc"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<BaseDescBinding>());
    result["equipment"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<EquipmentBinding>());
    result["inventory"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<InventoryBinding>());
    result["isDead"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<IsDeadBinding>());
    result["isDisabled"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<IsDisabledBinding>());
    result["isOnline"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<IsOnlineBinding>());
    result["isOpen"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<IsOpenBinding>());
    result["locationalData"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<LocationalDataBinding>());
    result["neighbors"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<NeighborsBinding>());
    result["onlinePlayers"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<OnlinePlayersBinding>());
    result["percentages"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<PercentagesBinding>());
    result["pos"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<PosBinding>());
    result["spawnPoint"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<SpawnPointBinding>());
    result["type"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<TypeBinding>());
    result["worldOrCellDesc"] = std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<WorldOrCellDescBinding>());
    return result;
}

std::shared_ptr<PropertyBinding> PropertyBindingFactory::CreateCustomPropertyBinding(const std::string &propertyName) {
    return std::dynamic_pointer_cast<PropertyBinding>(std::make_shared<CustomPropertyBinding>(propertyName));
}
