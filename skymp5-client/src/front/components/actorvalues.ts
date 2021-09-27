import {
    Actor,
} from "skyrimPlatform";

import * as structures from "../../lib/structures/actorvalues";
export type ActorValues = structures.ActorValues;

export const getActorValues = (ac: Actor): ActorValues => {
    if (!ac) return { health: 0, stamina: 0, magicka: 0 };
    let healthPercentage = (ac.isDead()) ? 0 : ac.getActorValuePercentage("health");
    const staminaPercentage = ac.getActorValuePercentage("stamina");
    const magickaPercentage = ac.getActorValuePercentage("magicka");

    const resultActorValue: ActorValues = {
        health: healthPercentage,
        stamina: staminaPercentage,
        magicka: magickaPercentage,
    };
    return resultActorValue;
}

export const setActorValuePercentage = (ac: Actor, avName: string, baseAV: number, percentage: number) =>
{
    const av = ac.getActorValue(avName);

    if(baseAV != av) {
      ac.restoreActorValue(avName, baseAV - av);
    }
    ac.damageActorValue(
      avName,
      baseAV - baseAV * percentage
    );
}
