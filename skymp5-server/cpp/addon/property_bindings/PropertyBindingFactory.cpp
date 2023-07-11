#include "PropertyBindingFactory.h"

#include "ActorNeighborsBinding.h"
#include "AngleBinding.h"
#include "AppearanceBinding.h"
#include "BaseDescBinding.h"
#include "ConsoleCommandsAllowedBinding.h"
#include "CustomPropertyBinding.h"
#include "EquipmentBinding.h"
#include "IdxBinding.h"
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
#include "ProfileIdBinding.h"
#include "SpawnPointBinding.h"
#include "TypeBinding.h"
#include "WorldOrCellDescBinding.h"

std::map<std::string, std::shared_ptr<PropertyBinding>>
PropertyBindingFactory::CreateStandardPropertyBindings()
{
  std::map<std::string, std::shared_ptr<PropertyBinding>> result;
  result["actorNeighbors"] = std::make_shared<ActorNeighborsBinding>();
  result["angle"] = std::make_shared<AngleBinding>();
  result["appearance"] = std::make_shared<AppearanceBinding>();
  result["baseDesc"] = std::make_shared<BaseDescBinding>();
  result["equipment"] = std::make_shared<EquipmentBinding>();
  result["inventory"] = std::make_shared<InventoryBinding>();
  result["isDead"] = std::make_shared<IsDeadBinding>();
  result["isDisabled"] = std::make_shared<IsDisabledBinding>();
  result["isOnline"] = std::make_shared<IsOnlineBinding>();
  result["isOpen"] = std::make_shared<IsOpenBinding>();
  result["locationalData"] = std::make_shared<LocationalDataBinding>();
  result["neighbors"] = std::make_shared<NeighborsBinding>();
  result["onlinePlayers"] = std::make_shared<OnlinePlayersBinding>();
  result["percentages"] = std::make_shared<PercentagesBinding>();
  result["pos"] = std::make_shared<PosBinding>();
  result["profileId"] = std::make_shared<ProfileIdBinding>();
  result["spawnPoint"] = std::make_shared<SpawnPointBinding>();
  result["type"] = std::make_shared<TypeBinding>();
  result["worldOrCellDesc"] = std::make_shared<WorldOrCellDescBinding>();
  result["idx"] = std::make_shared<IdxBinding>();
  result["consoleCommandsAllowed"] =
    std::make_shared<ConsoleCommandsAllowedBinding>();
  return result;
}

std::shared_ptr<PropertyBinding>
PropertyBindingFactory::CreateCustomPropertyBinding(
  const std::string& propertyName)
{
  return std::make_shared<CustomPropertyBinding>(propertyName);
}
