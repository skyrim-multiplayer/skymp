
import { ActivateMessage } from "./activateMessage";
import { ChangeValuesMessage } from "./changeValues";
import { ConsoleCommandMessage } from "./consoleCommandMessage";
import { CraftItemMessage } from "./craftItemMessage";
import { CustomEventMessage } from "./customEventMessage";
import { CustomPacketMessage } from "./customPacketMessage";
import { DropItemMessage } from "./dropItemMessage";
import { FinishSpSnippetMessage } from "./finishSpSnippetMessage";
import { HitMessage } from "./hitMessage";
import { HostMessage } from "./hostMessage";
import { OnEquipMessage } from "./onEquipMessage";
import { PutItemMessage } from "./putItemMessage";
import { TakeItemMessage } from "./takeItemMessage";
import { UpdateAnimationMessage } from "./updateAnimationMessage";
import { UpdateAppearanceMessage } from "./updateAppearanceMessage";
import { UpdateEquipmentMessage } from "./updateEquipmentMessage";
import { UpdateMovementMessage } from "./updateMovementMessage";

export type AnyMessage = ActivateMessage
    | ConsoleCommandMessage 
    | PutItemMessage 
    | TakeItemMessage 
    | CraftItemMessage
    | DropItemMessage
    | HitMessage
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
