import { Actor } from "skyrimPlatform";

export interface ApplyDeathStateEvent {
    actor: Actor;
    isDead: boolean;
}
