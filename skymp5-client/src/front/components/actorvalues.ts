import {
    Actor,
} from "skyrimPlatform";

import * as structures from "../../lib/structures/actorvalues";
export type ActorValues = structures.ActorValues;

export const getActorValues = (ac: Actor) : ActorValues => {

    let healthPercentage = ac && ac.getActorValuePercentage("health");
    if (ac && ac.isDead()) {
      healthPercentage = 0;
    }
    let staminaPercentage = ac && ac.getActorValuePercentage("stamina");
    let magickaPercentage = ac && ac.getActorValuePercentage("magicka");

    const resultActorValue: ActorValues = {
        health: healthPercentage,
        stamina: staminaPercentage,
        magicka: magickaPercentage,
    };
    return resultActorValue;
}
