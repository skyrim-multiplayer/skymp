import { Actor } from 'skyrimPlatform';

export interface ActorValues {
  health: number;
  stamina: number;
  magicka: number;
}

export const getActorValues = (ac: Actor): ActorValues => {
  if (!ac) return { health: 0, stamina: 0, magicka: 0 };
  let healthPercentage = ac.isDead() ? 0 : ac.getActorValuePercentage('health');
  const staminaPercentage = ac.getActorValuePercentage('stamina');
  const magickaPercentage = ac.getActorValuePercentage('magicka');

  const resultActorValue: ActorValues = {
    health: healthPercentage,
    stamina: staminaPercentage,
    magicka: magickaPercentage,
  };
  return resultActorValue;
};

export const getMaximumActorValue = (ac: Actor, avName: string): number => {
  const currentPercentage = ac.getActorValuePercentage(avName);
  return currentPercentage === 0
    ? ac.getBaseActorValue(avName)
    : Math.ceil(ac.getActorValue(avName) / currentPercentage);
};

export const setActorValuePercentage = (
  ac: Actor,
  avName: string,
  percentage: number,
): void => {
  // Actor value percentage for health may be below zero (-1.8 for example, it means u have -180% health)
  const currentPercentage = ac.getActorValuePercentage(avName);
  if (currentPercentage === percentage) return;

  const currentMaxValue = getMaximumActorValue(ac, avName);
  const deltaPercentage = percentage - currentPercentage;
  if (deltaPercentage > 0) {
    ac.restoreActorValue(avName, deltaPercentage * currentMaxValue);
  } else if (deltaPercentage < 0) {
    ac.damageActorValue(avName, deltaPercentage * currentMaxValue);
  }
};
