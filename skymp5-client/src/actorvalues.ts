import {
  Actor,
} from "skyrimPlatform";

export interface ActorValues {
  health: number;
  stamina: number;
  magicka: number;
}

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

export const setActorValuePercentage = (ac: Actor, avName: string, percentage: number) => {
  const currentPercentage = ac.getActorValuePercentage(avName);
  if (currentPercentage === percentage) return;

  const currentMax = ac.getBaseActorValue(avName);
  const deltaPercentage = percentage - currentPercentage;
  const k = 1;
  if (deltaPercentage > 0) {
    ac.restoreActorValue(avName, deltaPercentage * currentMax * k);
  } else if (deltaPercentage < 0) {
    ac.damageActorValue(avName, deltaPercentage * currentMax * k);
  }
};
