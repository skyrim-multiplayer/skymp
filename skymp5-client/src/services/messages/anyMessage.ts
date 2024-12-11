
import { ActivateMessage } from "./activateMessage";
import { ChangeValuesMessage } from "./changeValuesMessage";
import { ConsoleCommandMessage } from "./consoleCommandMessage";
import { CraftItemMessage } from "./craftItemMessage";
import { CreateActorMessage } from "./createActorMessage";
import { CustomEventMessage } from "./customEventMessage";
import { CustomPacketMessage } from "./customPacketMessage";
import { CustomPacketMessage2 } from "./customPacketMessage2";
import { DeathStateContainerMessage } from "./deathStateContainerMessage";
import { DestroyActorMessage } from "./destroyActorMessage";
import { DropItemMessage } from "./dropItemMessage";
import { FinishSpSnippetMessage } from "./finishSpSnippetMessage";
import { HitMessage } from "./hitMessage";
import { SpellCastMessage } from "./spellCastMessage";
import { UpdateAnimVariablesMessage } from "./updateAnimVariablesMessage";
import { HostMessage } from "./hostMessage";
import { HostStartMessage } from "./hostStartMessage";
import { HostStopMessage } from "./hostStopMessage";
import { OnEquipMessage } from "./onEquipMessage";
import { OpenContainerMessage } from "./openContainerMessage";
import { PlayerBowShotMessage } from "./playerBowShotMessage";
import { PutItemMessage } from "./putItemMessage";
import { SetInventoryMessage } from "./setInventoryMessage";
import { SetRaceMenuOpenMessage } from "./setRaceMenuOpenMessage";
import { SpSnippetMessage } from "./spSnippetMessage";
import { TakeItemMessage } from "./takeItemMessage";
import { TeleportMessage } from "./teleportMessage";
import { TeleportMessage2 } from "./teleportMessage2";
import { UpdateAnimationMessage } from "./updateAnimationMessage";
import { UpdateAppearanceMessage } from "./updateAppearanceMessage";
import { UpdateEquipmentMessage } from "./updateEquipmentMessage";
import { UpdateGamemodeDataMessage } from "./updateGameModeDataMessage";
import { UpdateMovementMessage } from "./updateMovementMessage";
import { UpdatePropertyMessage } from "./updatePropertyMessage";

export type AnyMessage = ActivateMessage
    | ConsoleCommandMessage
    | PutItemMessage
    | TakeItemMessage
    | CraftItemMessage
    | DropItemMessage
    | HitMessage
    | SpellCastMessage
    | UpdateAnimVariablesMessage
    | OnEquipMessage
    | UpdateMovementMessage
    | UpdateAnimationMessage
    | UpdateEquipmentMessage
    | ChangeValuesMessage
    | UpdateAppearanceMessage
    | HostMessage
    | CustomEventMessage
    | CustomPacketMessage
    | FinishSpSnippetMessage
    | TeleportMessage
    | OpenContainerMessage
    | HostStartMessage
    | HostStopMessage
    | SetInventoryMessage
    | CreateActorMessage
    | CustomPacketMessage2
    | DestroyActorMessage
    | SetRaceMenuOpenMessage
    | SpSnippetMessage
    | UpdateGamemodeDataMessage
    | UpdatePropertyMessage
    | DeathStateContainerMessage
    | TeleportMessage2
    | PlayerBowShotMessage
